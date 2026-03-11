#include "ble.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "uart_queue.h"

volatile int ble_cmd_state = 0;
volatile int ble_connect_state = 0;

void msleep(int ms) { usleep(ms * 1000); }

int uart_open_config(const char *dev, speed_t baud) {
  int fd = open(dev, O_RDWR | O_NOCTTY | O_NONBLOCK); // Open UART nonblocking
  if (fd < 0) {                                       // If open failed
    perror("open uart");                              // Print reason (errno)
    return -1;                                        // Error
  }

  struct termios tio;                                 // termios structure
  memset(&tio, 0, sizeof(tio));                        // Clear it

  if (tcgetattr(fd, &tio) != 0) {                      // Read current UART settings
    perror("tcgetattr");                               // Print error
    close(fd);                                         // Close fd
    return -1;                                         // Error
  }

  cfsetispeed(&tio, baud);                             // Set input baud
  cfsetospeed(&tio, baud);                             // Set output baud

  tio.c_cflag = (tio.c_cflag & ~CSIZE) | CS8;          // 8 data bits
  tio.c_cflag |= (CLOCAL | CREAD);                     // Local (ignore modem), enable receiver
  tio.c_cflag &= ~(PARENB | PARODD);                   // No parity
  tio.c_cflag &= ~CSTOPB;                              // 1 stop bit
  tio.c_cflag &= ~CRTSCTS;                             // No HW flow control

  tio.c_iflag = IGNPAR;                                // Ignore framing/parity errors (v1)
  tio.c_oflag = 0;                                     // Raw output
  tio.c_lflag = 0;                                     // Raw input (no canonical mode)

  tcflush(fd, TCIFLUSH);                               // Flush any stale input bytes
  if (tcsetattr(fd, TCSANOW, &tio) != 0) {              // Apply settings immediately
    perror("tcsetattr");                               // Print error
    close(fd);                                         // Close fd
    return -1;                                         // Error
  }

  return fd;                                           // Return UART fd
}

int ble_uart_write(int uart_fd, const char *str, int size)
{
    int result = write(uart_fd, str, size);
    if (result < 0)
    {
        perror("UART write failed");
        return -1;
    }
    return result;
}

int ble_enter_cmd(int uart_fd){
    char buffer[256];
    int n, results;

    ble_cmd_state = 0;

    if (ble_uart_write(uart_fd, "$$$", 3) < 0) return -1;

    msleep(25);

    n = uart_read_and_queue(uart_fd, buffer, sizeof(buffer));

    if (n > 0 && strstr(buffer, "CMD>")) {
        ble_cmd_state = 1;
        return 0;
    }

    if (ble_uart_write(uart_fd, "UNKNOWN\r", 8) < 0) return -1;

    msleep(25);

    n = uart_read_and_queue(uart_fd, buffer, sizeof(buffer));

    if (n > 0 && strstr(buffer, "Err") != NULL) {
        ble_cmd_state = 1;
        return 1;
    }

    return -1;
}

int ble_exit_cmd(int uart_fd) {
    char buffer[256];
    
    if (write(uart_fd, "TESTING\r", 8) != 8) return -1;
    msleep(10);
    

    if (write(uart_fd, "---\r", 5) != 5) return -1;
    msleep(500); 

    memset(buffer, 0, sizeof(buffer));
    if (read(uart_fd, buffer, sizeof(buffer) - 1) > 0) {
      printf("[PMOD RESPONSE]: %s\r\n", buffer);
      if (strstr(buffer, "END")) {
        ble_cmd_state = 0;
        return 0; 
      }
    }

    return -1; 
}


int ble_enter_client_mode(int uart_fd)
{
    
    const char *ci_cmd = "CI\r";
    const char *notify_cmd = "CHW,002B,0100\r";

    char buffer[128];
    ssize_t n;
    int attempts = 0;

    if (write(uart_fd, ci_cmd, strlen(ci_cmd)) != (ssize_t)strlen(ci_cmd)) {
        perror("Failed to send CI command");
        return -1;
    }

    //printf("Sent CI command, entering data mode\r\n");

    usleep(200000);   // 200 ms
    tcflush(uart_fd, TCIFLUSH);

    if (write(uart_fd, notify_cmd, strlen(notify_cmd)) != (ssize_t)strlen(notify_cmd)) {
        perror("Failed to send CHW command");
        return -1;
    }

    //printf("Enabling notifications\r\n");

    while (attempts < 10)
    {
        memset(buffer, 0, sizeof(buffer));

        n = read(uart_fd, buffer, sizeof(buffer) - 1);

        if (n > 0)
        {
            buffer[n] = '\0';
            printf("[PMOD RESPONSE]: %s\r\n", buffer);

            if (strstr(buffer, "AOK") != NULL) {
                printf("Notifications enabled successfully\r\n");
                return 1;
            }

            if (strstr(buffer, "ERR") != NULL) {
                printf("Failed to enable notifications\r\n");
                return -1;
            }
        }

        usleep(100000);
        attempts++;
    }

    printf("Timeout waiting for CHW response\r\n");
    return -1;
}

int ble_connect_mac(int uart_fd, const char *mac){
    if(ble_connect_state){
        return 2;
    }

    char cmd[64];
    char buffer[256];
    ssize_t n;
    int attempts = 0;

    if (!ble_cmd_state) {
        if (ble_enter_cmd(uart_fd) < 0) {
            //printf("Failed to enter BLE command mode\n");
            return -3;
        }
    }

    tcflush(uart_fd, TCIFLUSH);


    snprintf(cmd, sizeof(cmd), "C,0,%s\r", mac);

    if (write(uart_fd, cmd, strlen(cmd)) != (ssize_t)strlen(cmd)) {
        perror("UART write failed");
        return -1;
    }

    //printf("Connecting to %s...\n", mac);

    while (attempts < 20)
    {
        memset(buffer, 0, sizeof(buffer));

        n = read(uart_fd, buffer, sizeof(buffer) - 1);

        if (n > 0)
        {
            buffer[n] = '\0';

            //printf("[PMOD RESPONSE]: %s\r\n", buffer);

            /* Success case */
            if (strstr(buffer, "CONNECT") != NULL) {
                printf("BLE connection established\r\n");
                ble_connect_state = 1;
                if(ble_enter_client_mode(uart_fd)){ return 0; }
                return 1;
            }

            /* Error cases */
            if (strstr(buffer, "Err") != NULL) {
                //printf("BLE connection failed\r\n");
                return -1;
            }
        }

        usleep(200000);  // 200 ms
        attempts++;
    }

    printf("BLE connection timeout\r\n");
    return -2;
}

int ble_disconnect(int uart_fd) {
    if (!ble_connect_state) {
        printf("Connection Status: %d", ble_connect_state);
        return 1;
    }

    char buffer[128];
    ssize_t n;
    int attempts = 0;

    if (!ble_cmd_state) {
        if (ble_enter_cmd(uart_fd) < 0) {
            //printf("Failed to enter BLE command mode\n");
            return -3;
        }
    }

    if (write(uart_fd, "K,1\r", 4) != 4) {
        perror("UART write failed");
        return -1;
    }
    msleep(100);

    // Read response with a short retry loop
    while (attempts < 10) {
        memset(buffer, 0, sizeof(buffer));
        n = read(uart_fd, buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            printf("[PMOD RESPONSE]: %s\r\n", buffer);

            if (strstr(buffer, "DISCONNECT") != NULL) {
                ble_connect_state = 0;  // update state
                return 0;
            }

            if (strstr(buffer, "Err") != NULL) {
                return -2;
            }
        }
        msleep(100);
        attempts++;
    }

    // If we get here, we timed out waiting for DISCONNECT
    return -1;
}


//TODO BLE INIT
int ble_init(int uart_fd){
    uart_queue_init(&uart_queue);

    ble_enter_cmd(uart_fd);
    if (write(uart_fd, "K,1\r", 4) != 4) {
        perror("UART write failed");
        return -1;
    }
    msleep(100);
    return 0;
}