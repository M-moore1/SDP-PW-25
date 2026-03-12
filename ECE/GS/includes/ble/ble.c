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
    int n;

    ble_cmd_state = 0;

    if (ble_uart_write(uart_fd, "$$$", 3) < 0) return -1;

    msleep(500);

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
    if (ble_cmd_state == 0){
        return 0;
    }

    char buffer[256];
    if (ble_uart_write(uart_fd, "TESTING\r", 8) < 0) return -1;
    msleep(150);
    
    if (ble_uart_write(uart_fd, "---\r", 5) < 0) return -1;
    msleep(500); 

    int n = uart_read_and_queue(uart_fd, buffer, sizeof(buffer));
    if (n > 0 && strstr(buffer, "END") != NULL){
        ble_cmd_state = 0;
        return 0; 
    }

    return -1; 
}

int ble_reset(int uart_fd){
    char buffer[256];
    int n;

    if (ble_uart_write(uart_fd, "\rR,1\r", 5) < 0) return -1;
    msleep(500);

    n = uart_read_and_queue(uart_fd, buffer, sizeof(buffer));
    if (n > 0 && strstr(buffer, "REBOOT") != NULL) {
        return ble_enter_cmd(uart_fd);
    }

    return -1;
}

int ble_enter_client_mode(int uart_fd)
{
    const char *ci_cmd = "CI\r";
    const char *notify_cmd = "CHW,002B,0100\r"; // Notification Turn On CMD

    char buffer[128];
    ssize_t n;
    int attempts = 0;


    if (ble_uart_write(uart_fd,  ci_cmd, strlen(ci_cmd)) < 0) return -1;
    //printf("Sent CI command, entering data mode\r\n");
    msleep(200); 

    if (ble_uart_write(uart_fd,  notify_cmd, strlen(notify_cmd)) < 0) return -1;
    //printf("Enabling notifications\r\n");

    while (attempts < 10)
    {
        n = uart_read_and_queue(uart_fd, buffer, sizeof(buffer));
        if (n > 0)
        {
            buffer[n] = '\0';

            if (strstr(buffer, "AOK") != NULL) {
                printf("Notifications enabled successfully\r\n");
                return 0;
            }

            if (strstr(buffer, "ERR") != NULL) {
                printf("Failed to enable notifications\r\n");
                return -1;
            }
        }

        msleep(100); 
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
        ble_enter_cmd(uart_fd);
    }

    snprintf(cmd, sizeof(cmd), "C,0,%s\r", mac);

    if (ble_uart_write(uart_fd, cmd, strlen(cmd)) < 0) return -1;
    //printf("Connecting to %s...\n", mac);

    while (attempts < 20)
    {
        n = uart_read_and_queue(uart_fd, buffer, sizeof(buffer));

        if (n > 0)
        {
            buffer[n] = '\0';

            if (strstr(buffer, "CONNECT") != NULL) {
                if (strstr(buffer, "DISCONNECT") != NULL){
                    return -1;
                }
                if(!ble_enter_client_mode(uart_fd)){ 
                    ble_connect_state = 1;
                    return 0; 
                }
                return 1;
            }

            if (strstr(buffer, "Err") != NULL) {
                return -1;
            }
        }

        msleep(1000); 
        attempts++;
    }

    return -2;
}

int ble_disconnect(int uart_fd) {

    char buffer[128];
    ssize_t n;
    int attempts = 0;

    if (!ble_cmd_state) {
        ble_enter_cmd(uart_fd);
    }

    if (ble_uart_write(uart_fd, "K,1\r", 4) < 0) return -1;
    msleep(100);

    // Read response with a short retry loop
    while (attempts < 10) {
        n = uart_read_and_queue(uart_fd, buffer, sizeof(buffer));

        if (n > 0) {
            buffer[n] = '\0';
            if (strstr(buffer, "DISCONNECT") != NULL) {
                ble_connect_state = 0;  // update state
                msleep(2000);
                return 0;
            }

            if (strstr(buffer, "Err") != NULL) {
                return -2;
            }
        }
        msleep(100);
        attempts++;
    }

    return -1;
}

int ble_connect_check(int uart_fd)
{
    char buffer[128];
    ssize_t n;

    if (!ble_cmd_state) {
        ble_enter_cmd(uart_fd);
    }
    msleep(1000);

    if (ble_uart_write(uart_fd, "\rGK\r", 4)  < 0) return -1;
    msleep(10);

    n = uart_read_and_queue(uart_fd, buffer, sizeof(buffer));
    if (n > 0) {
        buffer[n] = '\0';
        if (strstr(buffer, "none") != NULL) {
            ble_connect_state = 0;
            return -1;
        }
        ble_connect_state = 0;
        return 0;
    }
    return -1;
}

int ble_init(int uart_fd){
    
    uart_queue_init(&uart_queue);
    ble_reset(uart_fd);

    return 0;
}

int uart_send_str(int uart_fd, char * str, int str_len){
    const char *start_write = "\rCHW,002A,";
    const char *enter = "\r,";
    if (ble_uart_write(uart_fd,  start_write, strlen(start_write)) < 0) return -1;
    
    if (ble_uart_write(uart_fd,  str, str_len) < 0) return -1;
    if (ble_uart_write(uart_fd,  enter, strlen(enter)) < 0) return -1;


}

