// gs_bridge.c
// -----------------------------------------------------------------------------
// Bridge daemon:
//   Node.js <-> Unix Domain Socket (JSON, length-prefixed) <-> C bridge
//   C bridge <-> UART (BT2/RN-42 SPP) (binary framed 64-bit payload) <-> ESP32
//
// UART frame format (v1):
//   [0]=0xAA [1]=0x55 [2]=len(=8) [3..10]=payload(8 bytes, big-endian) [11]=xor
//
// UDS frame format:
//   4-byte big-endian length, then JSON bytes
//
// Build example:
//   make all
//   make clean
// -----------------------------------------------------------------------------

#define _GNU_SOURCE                     // Enables some GNU extensions (safe on Linux)
#include <arpa/inet.h>                  // htonl/ntohl for endian conversion
#include <errno.h>                      // errno and error codes
#include <fcntl.h>                      // open(), fcntl() flags
#include <stdint.h>                     // uint8_t/uint16_t/uint32_t/uint64_t
#include <stdio.h>                      // printf(), perror()
#include <stdlib.h>                     // malloc(), free(), getenv()
#include <string.h>                     // memset(), memcpy(), strncpy(), strcmp()
#include <sys/select.h>                 // select()
#include <sys/socket.h>                 // socket(), bind(), listen(), accept()
#include <sys/stat.h>                   // chmod()
#include <sys/un.h>                     // sockaddr_un for Unix domain sockets
#include <termios.h>                    // termios UART config
#include <unistd.h>                     // read(), write(), close(), unlink()
#include "includes/cmd_structure.h"
#include "../includes/ble/pmod_esp32.h"
#include "includes/ble/uart_queue.h"
#include "includes/cmd_parser/cmd_parser.h"
#include "includes/json_uds/json_uds.h"

#include "cJSON.h"                     // cJSON library header (vendored)
// 004B1224B0A6
// ------------------------- Defaults / Config -------------------------

#define DEFAULT_UDS_PATH "/tmp/gs_bridge.sock" // Socket file path for Node<->C IPC
#define DEFAULT_UART_DEV "/dev/ttyPS2"         // Default UART device (Zynq PS UART)


int looks_like_json(const char *s) {
  if (!s) return 0;
  while (*s == ' ' || *s == '\n' || *s == '\r' || *s == '\t') s++;
  return (*s == '{' || *s == '[');
}


// ------------------------- Main -------------------------

int main(int argc, char **argv) {
  setvbuf(stdout, NULL, _IOLBF, 0);  // Line-buffer stdout immediately

  const char *uart_dev = DEFAULT_UART_DEV;
  const char *env_uart = getenv("UART_DEV");
  if (env_uart && env_uart[0]) uart_dev = env_uart;
  if (argc >= 2) uart_dev = argv[1];

  printf("Hello — uart_dev=%s\n", uart_dev);  // Will now appear

  int uart_fd = uart_open_config(uart_dev, DEFAULT_UART_BAUD);
  if (uart_fd < 0) {
    fprintf(stderr, "ERROR: uart_open_config(%s) failed: %s\n",
            uart_dev, strerror(errno));
    return 1;
  }
  printf("UART opened: fd=%d\n", uart_fd);
  // ...
  ble_init(uart_fd);
  
  const char *uds_path = DEFAULT_UDS_PATH;                 // UDS path (could also make configurable)

  int uds_listen = uds_server_listen(uds_path);            // Create UDS listening socket
  if (uds_listen < 0) return 1;                            // If failed, exit

  printf("Bridge up. UDS=%s UART=%s\n", uds_path, uart_dev);// Helpful startup message

  int uds_client = -1;                                     // Node client fd (none yet)
  int bt_connect_attempted = 0;

  while (1) {                                              // Run forever
    if (uds_client < 0) {                                  // If Node not connected
      printf("Waiting for Node connection...\n");          // Tell user
      uds_client = accept(uds_listen, NULL, NULL);         // Accept one client
      if (uds_client < 0) { perror("accept"); continue; }  // Retry on failure
      fcntl(uds_client, F_SETFL, O_NONBLOCK);              // Make Node socket nonblocking
      printf("Node connected.\n");                         // Confirm

      //ONLY CONNECT ONCE BASED ON UI CONNECTION MAYBE REMOVE TO LET UI HAVE FULL CONTROL
      // Connect RN-42 to ESP32 now that Node is connected
      // Try RN-42 connect ONCE after Node connects
      printf("%d", bt_connect_attempted);
      if (!bt_connect_attempted) {
        bt_connect_attempted = 1;

        printf("%d", bt_connect_attempted);


        const char *esp32_mac = ESP32_MAC;  
        
        printf("Entering cmd\n");
        printf("BLE: connecting to ESP32 MAC %s...\n", esp32_mac);
        if (ble_connect(uart_fd, esp32_mac) != 0) {
          printf("BLE: connect attempt failed (will not retry unless Node reconnects)\n");
        } else {
          printf("BLE: connect command sent.\n");
        }
        
      }
    }

    fd_set rfds;                                           // Read fd set
    FD_ZERO(&rfds);                                        // Clear set
    FD_SET(uds_client, &rfds);                             // Watch Node socket
    FD_SET(uart_fd, &rfds);                                // Watch UART
    int maxfd = (uds_client > uart_fd) ? uds_client : uart_fd; // select needs max+1

    struct timeval tv;                                     // Timeout for select()
    tv.tv_sec = 0;                                         // 0 seconds
    tv.tv_usec = 20000;                                    // 20ms tick (gives responsiveness)

    int rc = select(maxfd + 1, &rfds, NULL, NULL, &tv);     // Wait for readable fd(s)
    if (rc < 0) {                                          // Error
      if (errno == EINTR) continue;                        // Interrupted -> retry
      perror("select");                                    // Print error
      break;                                               // Exit loop
    }

    // ----- If Node socket readable: read 1 framed JSON message -----

    if (FD_ISSET(uds_client, &rfds)) {         
                  // Node has data
      uint32_t len_be = 0;                                 // 4-byte big-endian length
      int r = read_full(uds_client, &len_be, 4);           // Read length
      if (r == 0) {                                        // Node disconnected
        close(uds_client);                                 // Close client fd
        uds_client = -1;                                   // Mark as disconnected
        continue;                                          // Go back to accept()
      }
      if (r < 0) {
        // -2 would mean "would block" in nonblocking mode; ignore for now.
      } else {
        uint32_t len = ntohl(len_be);                      // Convert length to host endian
        if (len == 0 || len > (1024*1024)) {               // Sanity check (max 1MB)
          close(uds_client);                               // Drop connection if invalid
          uds_client = -1;
          continue;
        }

        char *buf = (char*)malloc(len + 1);                // Allocate buffer
        if (!buf) {                                        // OOM
          close(uds_client);
          uds_client = -1;
          continue;
        }

        int r2 = read_full(uds_client, buf, len);          // Read JSON payload bytes
        if (r2 <= 0) {                                     // Error/disconnect
          free(buf);
          close(uds_client);
          uds_client = -1;
          continue;
        }

        buf[len] = '\0';                                   // Null-terminate JSON string
        int handled = 0;

        // Fast path: if it looks like JSON and parses, treat as plaintext JSON
        

        if (looks_like_json(buf)) {
          printf("I GOT a JSON\r\n");
          cJSON *probe = cJSON_Parse(buf);
          if (probe) {
            cJSON_Delete(probe);
            printf("UDS->C plaintext JSON\n");
            handle_node_json(uart_fd, uds_client, buf);
            handled = 1;
          }
        }
        // If not handled, attempt decrypt path or send raw string over Bluetooth
        if (!handled) {
          printf("I GOT SOMETHING ENCRYPTED\r\n");
          handle_encrypted_data(uart_fd, uds_client, buf);
        }

        //printf("\r\n Recieved Data:\r\n");
        //printf("%s \r\n", buf);

        free(buf);                                         // Free buffer
      }
    }

    // ----- If UART readable: read bytes, parse frames, forward to Node -----
    /*
    if (FD_ISSET(uart_fd, &rfds)) {                         // UART has data
      uint8_t tmp[256];                                     // Temp read buffer
      ssize_t n = read(uart_fd, tmp, sizeof(tmp));          // Read available bytes
      if (n > 0) {                                          // If bytes received
        for (ssize_t i = 0; i < n; i++) {                   // Feed each byte into parser
          uint64_t word = 0;                                // Output word
          //int got = uart_parser_feed(&up, tmp[i], &word);   // Parser state update
          //if (got == 1 && uds_client >= 0) {                // If we got a complete frame
            //robot_word_to_node_json(uds_client, word);      // Convert to JSON and send to Node
          //}
        }
      }
    }
      */
    ble_uart_check(uart_fd);
    char rx_buffer[256]; 
    if (uart_queue_pop(&uart_queue, rx_buffer) == 0){
       printf("[UART OUTPUT] %s\r\n", rx_buffer);
    }
  }

  // Cleanup on exit
  if (uds_client >= 0) close(uds_client);                   // Close Node connection
  close(uds_listen);                                        // Close UDS server
  close(uart_fd);                                           // Close UART
  unlink(uds_path);                                         // Remove socket file

  return 0;                                                 // Exit
}