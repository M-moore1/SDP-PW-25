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
#include <time.h>
// 004B1224B0A6
// ------------------------- Defaults / Config -------------------------

#define DEFAULT_UDS_PATH "/tmp/gs_bridge.sock" // Socket file path for Node<->C IPC
#define DEFAULT_UART_DEV "/dev/ttyPS2"         // Default UART device (Zynq PS UART)

#define MAX_TRACKED_CMDS 64

typedef struct {
    uint16_t id;
    uint64_t timestamp_ms;
    int valid;
} cmd_tracker_t;

static cmd_tracker_t cmd_buffer[MAX_TRACKED_CMDS];

uint64_t get_now_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000ULL) + (ts.tv_nsec / 1000000ULL);
}

void store_command(uint16_t id) {

    uint64_t now = get_now_ms();

    for (int i = 0; i < MAX_TRACKED_CMDS; i++) {
        if (!cmd_buffer[i].valid) {
            cmd_buffer[i].id = id;
            cmd_buffer[i].timestamp_ms = now;
            cmd_buffer[i].valid = 1;
            return;
        }
    }

    // Optional: overwrite oldest if full
    cmd_buffer[0].id = id;
    cmd_buffer[0].timestamp_ms = now;
    cmd_buffer[0].valid = 1;
}

void process_received_id(uint16_t id, int uds_fd) {

    uint64_t now = get_now_ms();

    for (int i = 0; i < MAX_TRACKED_CMDS; i++) {

        if (cmd_buffer[i].valid && cmd_buffer[i].id == id) {

            uint64_t latency = now - cmd_buffer[i].timestamp_ms;

            cmd_buffer[i].valid = 0;

            // Send to UI
            cJSON *root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "type", "LATENCY");
            cJSON_AddNumberToObject(root, "id", id);
            cJSON_AddNumberToObject(root, "latency_ms", latency);

            char *json = cJSON_PrintUnformatted(root);
            uds_send_json(uds_fd, json);

            free(json);
            cJSON_Delete(root);

            return;
        }
    }

    printf("ID not found: %d\n", id);
}


void parse_notify_and_process(char *line, int uds_fd) {

    int conn, srv, chr, ascii_len;

    if (sscanf(line, "+NOTIFY:%d,%d,%d,%d,", &conn, &srv, &chr, &ascii_len) != 4)
        return;

    // Move to hex data
    char *p = line;
    for (int i = 0; i < 4; i++) {
        p = strchr(p, ',');
        if (!p) return;
        p++;
    }

    int byte_len = ascii_len / 2;

    uint8_t data[512];

    for (int i = 0; i < byte_len; i++) {
        sscanf(&p[i * 2], "%2hhx", &data[i]);
    }

    // Strip start/stop
    if (data[0] != 0x0A || data[byte_len-1] != 0x0D)
        return;

    uint8_t *payload = &data[1];

    robot_bt_packet_t pkt;
    memcpy(pkt.bytes, payload, 8);

    // Extract ID
    uint16_t id = 0;

    switch(pkt.ctrl.type) {
        case System_CMD:
            id = pkt.sys.id;
            break;

        case Query_CMD:
            id = pkt.query.id;
            break;

        case ACK_CMD:
            id = pkt.ack.id;
            break;
    }

    if (id != 0) {
        process_received_id(id, uds_fd);
    }
}
int looks_like_json(const char *s) {
  if (!s) return 0;
  while (*s == ' ' || *s == '\n' || *s == '\r' || *s == '\t') s++;
  return (*s == '{' || *s == '[');
}


// ------------------------- Main -------------------------

int main(int argc, char **argv) {
  // Allow UART device override:
  // 1) environment variable UART_DEV
  // 2) command line argument (argv[1])
  // 3) default constant
  const char *uart_dev = DEFAULT_UART_DEV;                 // Default
  const char *env_uart = getenv("UART_DEV");               // Check env var
  if (env_uart && env_uart[0]) uart_dev = env_uart;        // Use env var if set
  if (argc >= 2) uart_dev = argv[1];                       // Use CLI arg if provided

  const char *uds_path = DEFAULT_UDS_PATH;                 // UDS path (could also make configurable)

  int uart_fd = uart_open_config(uart_dev, DEFAULT_UART_BAUD); // Open/config UART
  if (uart_fd < 0) return 1;                               // If failed, exit
  if (ble_init(uart_fd) < 0) return 1; // Init BLE
  

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
    if (FD_ISSET(uds_client, &rfds)) {                     // Node has data
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
          //printf("I GOT a JSON\r\n");
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

        printf("\r\n Recieved Data:\r\n");
        printf("%s \r\n", buf);

        free(buf);                                         // Free buffer
      }
    }

    // ----- If UART readable: read bytes, parse +NOTIFY lines, process -----
    if (FD_ISSET(uart_fd, &rfds)) {

        uint8_t tmp[256];
        ssize_t n = read(uart_fd, tmp, sizeof(tmp));

        if (n > 0) {

            static char uart_line[4096];
            static int uart_idx = 0;

            for (ssize_t i = 0; i < n; i++) {

                char c = tmp[i];

                // End of line → process full message
                if (c == '\n') {

                    uart_line[uart_idx] = '\0';

                    // DEBUG (optional)
                    printf("[UART LINE] %s\n", uart_line);

                    // Check for BLE notification
                    if (strstr(uart_line, "+NOTIFY:") != NULL) {
                        parse_notify_and_process(uart_line, uds_client);
                    }

                    uart_idx = 0;
                }
                else {
                    if (uart_idx < sizeof(uart_line) - 1) {
                        uart_line[uart_idx++] = c;
                    }
                }
            }
        }
    }
  }

  // Cleanup on exit
  if (uds_client >= 0) close(uds_client);                   // Close Node connection
  close(uds_listen);                                        // Close UDS server
  close(uart_fd);                                           // Close UART
  unlink(uds_path);                                         // Remove socket file

  return 0;                                                 // Exit
}