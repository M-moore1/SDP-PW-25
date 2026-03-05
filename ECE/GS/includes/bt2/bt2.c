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
  int n, results;

  results = write(uart_fd, "$$$", 3);
  if (results < 0) {
    perror("UART write failed");
    return -1;
  }
  msleep(500);

  memset(buffer, 0, sizeof(buffer));
  n = read(uart_fd, buffer, sizeof(buffer) - 1);
  //printf("[PMOD RESPONSE]: %s\r\n", buffer);
  
  if (n > 0 && strstr(buffer, "CMD")) {
    return 0; 
  }

  results = write(uart_fd, "UNKNOWN\r", 8); 
  if (results < 0) {
    perror("UART write failed");
    return -1;
  }
  msleep(200);

  memset(buffer, 0, sizeof(buffer));
  n = read(uart_fd, buffer, sizeof(buffer) - 1);
  //printf("[PMOD RESPONSE]: %s\r\n", buffer);

  if (n > 0 && strstr(buffer, "?")) {
    return 1; 
  }

  return -1; 
}

int rn42_exit_cmd(int uart_fd) {
    char buffer[16];
    
    // Clear any pending input to avoid confusion with the "EXIT" response
    if (write(uart_fd, "TESTING\r", 8) != 8) return -1;
    msleep(100);
    
    // Clear the local UART read buffer so '?' doesn't 
    tcflush(uart_fd, TCIFLUSH);

    if (write(uart_fd, "---\r", 5) != 5) return -1;
    msleep(500); 

    memset(buffer, 0, sizeof(buffer));
    if (read(uart_fd, buffer, sizeof(buffer) - 1) > 0) {
      //printf("[PMOD RESPONSE]: %s\r\n", buffer);
      if (strstr(buffer, "END")) {
        return 0; 
      }
    }

    return -1; 
}

int rn42_connect_mac(int uart_fd, const char *mac) {
  char cmd[64];
  char buffer[128];
  int attempts = 0;
  ssize_t n;

  if (rn42_enter_cmd(uart_fd) < 0) return -1;
  tcflush(uart_fd, TCIFLUSH);

  snprintf(cmd, sizeof(cmd), "C,%s\r", mac);
  if (write(uart_fd, cmd, strlen(cmd)) != (ssize_t)strlen(cmd)) {
    return -1;
  }

  //printf("Connecting to %s\r\n", mac);

  fflush(stdout);

  // Loop until timeout 
  while (attempts < 100) {
    memset(buffer, 0, sizeof(buffer));
    n = read(uart_fd, buffer, sizeof(buffer) - 1);

    if (n > 0) {
      //printf("\r\n[PMOD Response]: %s\r\n", buffer);

      if (strstr(buffer, "TRYING")) {
        //printf("Attempting to Connect.");
      }

      if (strstr(buffer, "CONNECT failed") || strstr(buffer, "ERR") || strstr(buffer, "?")) {
        //printf("Connection FAILED\r\n");
        rn42_exit_cmd(uart_fd);
        return -2; 
      }

      if (strstr(buffer, "CONNECT")) {
        //printf("\nResult: Connection SUCCESSFUL\r\n");
        return 0; // RN42 exits CMD mode automatically on success
      }
    }

    msleep(100); 
    attempts++;
    if (attempts % 10 == 0) { printf("."); fflush(stdout); }
  }

  //printf("Connection Timeout.\r\n");
  rn42_exit_cmd(uart_fd);
  return -3;
}

int rn42_disconnect(int uart_fd) {
  char buffer[64];

  if (rn42_enter_cmd(uart_fd) != 0) return -1;
  tcflush(uart_fd, TCIFLUSH);

  uart_write_str(uart_fd, "K,1\r");
  msleep(400);

  memset(buffer, 0, sizeof(buffer));
  if (read(uart_fd, buffer, sizeof(buffer) - 1) > 0) {
      //printf("\n[PMOD RESPONSE]: %s\r\n", buffer);
      if (strstr(buffer, "ERR") || strstr(buffer, "?")) {
          rn42_exit_cmd(uart_fd);
          return -2;
      }
  }
  msleep(3000);
  rn42_exit_cmd(uart_fd);
  return 0;
}

int rn42_connect_check(int uart_fd){
  char buffer[64];

  if (rn42_enter_cmd(uart_fd) != 0) return -1;
  tcflush(uart_fd, TCIFLUSH);

  uart_write_str(uart_fd, "GK\r");
  msleep(400);

  memset(buffer, 0, sizeof(buffer));
  if (read(uart_fd, buffer, sizeof(buffer) - 1) > 0) {
    //printf("\n[PMOD RESPONSE]: %s\n\r", buffer);
    if (strstr(buffer, "ERR") || strstr(buffer, "?")) {
      rn42_exit_cmd(uart_fd);
      return -1;
    }

    if (strstr(buffer, "0,0,0")) {
      rn42_exit_cmd(uart_fd);
      return 1; 
    }

    if (strstr(buffer, "1,0,0")) {
      rn42_exit_cmd(uart_fd);
      return 0; 
    }

  }

  rn42_exit_cmd(uart_fd);
  return -1;
}

int uart_send_str(int uart_fd, char *str) {
  char buffer[128]; 
  size_t len = strlen(str);

  // Ensure string + '\r' fits in buffer
  if (len > (sizeof(buffer) - 2)) {
    return -1; 
  }

  strcpy(buffer, str);

  // Append Carriage Return and Null Terminator
  buffer[len] = '\r';
  buffer[len + 1] = '\0';

  size_t total_to_send = len + 1;
  ssize_t n = write(uart_fd, buffer, total_to_send);

  
  if (n < 0) {
      return -1;
  }

  // Check if all bytes were actually sent 
  if ((size_t)n == total_to_send) {
    return 0;
  } else {
    return -1;
  }
}

int uart_send_encrypted(int uart_fd, uint8_t *encrypt_data){
  // DO NOT CHANGE WITHOUT TALKING
    uint8_t packet[158];
    memset(packet, 0, sizeof(packet));

    packet[0] = 0x0A;
    memcpy(&packet[1], encrypt_data, 156);
    packet[157] = 0x0D;

    ssize_t n = write(uart_fd, packet, sizeof(packet));
    
    if (n < 0) {
      perror("UART Write Error");
      return -1;
    }

    return 0;
}


int uart_send_instruction(int uart_fd, uint64_t instruction) {
  // DO NOT CHANGE WITHOUT TALKING
    uint8_t packet[158];
    memset(packet, 0, sizeof(packet));

    packet[0] = 0x0A;
    memcpy(&packet[1], &instruction, 8);
    packet[157] = 0x0D;

    ssize_t n = write(uart_fd, packet, sizeof(packet));
    if (n < 0) {
        perror("UART Write Error");
        return -1;
    }
    if (n != (ssize_t)sizeof(packet)) {
        fprintf(stderr, "Short write: sent %zd of %zu bytes\n", n, sizeof(packet));
        return -1;
    }

    return 0;
}
