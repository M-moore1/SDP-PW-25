#include "bt2.h"

// ------------------------- UART open/config -------------------------
// Open UART device and configure baud/8N1/no flow control (typical SPP serial).

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

void msleep(int ms) { usleep(ms * 1000); }

int uart_write_str(int fd, const char *s) {
  ssize_t n = write(fd, s, strlen(s));
  return (n == (ssize_t)strlen(s)) ? 0 : -1;
}

int rn42_enter_cmd(int uart_fd) {
    char buffer[32];
    int n;

    write(uart_fd, "$$$", 3);
    msleep(500);

    memset(buffer, 0, sizeof(buffer));
    n = read(uart_fd, buffer, sizeof(buffer) - 1);
    printf("\nPMOD Response: %s\n", buffer);
    
    if (n > 0 && strstr(buffer, "CMD")) {
        return 0; 
    }

    write(uart_fd, "UNKNOWN\r", 8); 
    msleep(200);

    memset(buffer, 0, sizeof(buffer));
    n = read(uart_fd, buffer, sizeof(buffer) - 1);
    printf("\nPMOD Response: %s\n", buffer);

    if (n > 0 && strstr(buffer, "?")) {
        return 1; 
    }

    return -1; 
}

int rn42_exit_cmd(int uart_fd) {
    char buffer[16];
    
    if (write(uart_fd, "\r", 1) != 1) return -1;
    msleep(100);
    
    // Optional: Clear the local UART read buffer so '?' doesn't 
    tcflush(uart_fd, TCIFLUSH);

    if (write(uart_fd, "---\r", 4) != 4) return -1;
    msleep(500); 

    memset(buffer, 0, sizeof(buffer));
    if (read(uart_fd, buffer, sizeof(buffer) - 1) > 0) {
      printf("\nPMOD Response: %s\n", buffer);
      if (strstr(buffer, "END")) {
        return 0; 
      }
    }

    return 0; 
}

int rn42_connect_mac(int uart_fd, const char *mac) {
    char cmd[64];
    char buffer[64];

    if (rn42_enter_cmd(uart_fd) < 0) return -1;

    snprintf(cmd, sizeof(cmd), "C,%s\r", mac);
    if (write(uart_fd, cmd, strlen(cmd)) != (ssize_t)strlen(cmd)) {
        return -1;
    }

    
    msleep(200);
    memset(buffer, 0, sizeof(buffer));
    if (read(uart_fd, buffer, sizeof(buffer) - 1) > 0) {
      printf("\nPMOD Response: %s\n", buffer);
      if (strstr(buffer, "ERR") || strstr(buffer, "?")) {
        rn42_exit_cmd(uart_fd);
        return -2;
      }
    }

    msleep(3000);

    rn42_exit_cmd(uart_fd);

    return 0;
}

int rn42_disconnect(int uart_fd) {
  char buffer[64];

  if (rn42_enter_cmd(uart_fd) != 0) return -1;

  uart_write_str(uart_fd, "K,1\r");
  msleep(400);

  memset(buffer, 0, sizeof(buffer));
  if (read(uart_fd, buffer, sizeof(buffer) - 1) > 0) {
      printf("\nPMOD Response: %s\n", buffer);
      if (strstr(buffer, "ERR") || strstr(buffer, "?")) {
          rn42_exit_cmd(uart_fd);
          return -2;
      }
  }
  msleep(3000);
  rn42_exit_cmd(uart_fd);

  return 0;
}


int uart_send_str(int uart_fd, char *str) {
    char buffer[128]; 
    int len = strlen(str);

    if (len > (sizeof(buffer) - 2)) {
        return -1; 
    }

    strcpy(buffer, str);

    buffer[len] = '\r';
    buffer[len + 1] = '\0';

    ssize_t n = write(uart_fd, buffer, len + 1);

    if (n < 0) {
        return -1;
    }

    return (n == (len + 1)) ? 0 : -1;
}

int uart_send_instruction(int uart_fd, uint64_t instruction) {
  printf("PMOD SENDING: %" PRIu64 "\n", instruction);

  uint8_t packet[9];
  memcpy(packet, &instruction, 8);
  packet[8] = 0x0D; // STOP BIT

  ssize_t n = write(uart_fd, packet, 9);
  
  if (n < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return 0; // Non-blocking: buffer full
    }
    perror("UART Write Error");
    return -1; 
  }

  if (n != 9) {
    fprintf(stderr, "Short write: sent %zd of 9 bytes\n", n);
    return -1; 
  }

  return 0;
}

