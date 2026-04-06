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




int looks_like_json(const char *s) {
  if (!s) return 0;
  while (*s == ' ' || *s == '\n' || *s == '\r' || *s == '\t') s++;
  return (*s == '{' || *s == '[');
}


// ------------------------- Main -------------------------
int main(int argc, char **argv) {

  // -------------------------------
  // UART DEVICE SELECTION
  // Priority:
  //   1) Environment variable UART_DEV
  //   2) Command line argument
  //   3) Default constant
  // -------------------------------
  const char *uart_dev = DEFAULT_UART_DEV;
  const char *env_uart = getenv("UART_DEV");

  if (env_uart && env_uart[0]) uart_dev = env_uart;
  if (argc >= 2) uart_dev = argv[1];

  // -------------------------------
  // UDS SOCKET PATH (Node.js <-> C bridge)
  // -------------------------------
  const char *uds_path = DEFAULT_UDS_PATH;

  // -------------------------------
  // UART INITIALIZATION
  // Opens and configures UART for communication with ESP32
  // -------------------------------
  int uart_fd = uart_open_config(uart_dev, DEFAULT_UART_BAUD);
  if (uart_fd < 0) return 1;

  // -------------------------------
  // BLE INITIALIZATION
  // Sets up ESP32 BLE module via AT commands
  // -------------------------------
  if (ble_init(uart_fd) < 0) return 1;

  // -------------------------------
  // UDS SERVER SETUP
  // Creates Unix Domain Socket for Node.js connection
  // -------------------------------
  int uds_listen = uds_server_listen(uds_path);
  if (uds_listen < 0) return 1;

  printf("Bridge up. UDS=%s UART=%s\n", uds_path, uart_dev);

  int uds_client = -1;             // Active Node.js connection (none initially)
  int bt_connect_attempted = 0;    // Ensures BLE connect only happens once

  // -------------------------------
  // MAIN EVENT LOOP (SELECT-BASED)
  // Handles FULL DUPLEX communication:
  //   - UI → Robot (UDS → UART → BLE)
  //   - Robot → UI (BLE → UART → UDS)
  // -------------------------------
  while (1) {

    // -------------------------------
    // ACCEPT NODE CONNECTION
    // Only one client allowed at a time
    // -------------------------------
    if (uds_client < 0) {
      printf("Waiting for Node connection...\n");

      uds_client = accept(uds_listen, NULL, NULL);
      if (uds_client < 0) {
        perror("accept");
        continue;
      }

      // Set non-blocking mode so select() works properly
      fcntl(uds_client, F_SETFL, O_NONBLOCK);

      printf("Node connected.\n");

      // -------------------------------
      // BLE CONNECTION (RUNS ONCE)
      // Connect ESP32 after UI connects
      // NOTE: Could be upgraded later to reconnect dynamically
      // -------------------------------
      if (!bt_connect_attempted) {
        bt_connect_attempted = 1;

        printf("BLE: connecting to ESP32 MAC %s...\n", ESP32_MAC);

        if (ble_connect(uart_fd, ESP32_MAC) != 0) {
          printf("BLE: connect failed\n");
        } else {
          printf("BLE: connected\n");
        }
      }
    }

    // -------------------------------
    // SELECT SETUP (SAFE VERSION)
    // FIX: Only add uds_client if valid (prevents FD_SET(-1) bug)
    // -------------------------------
    fd_set rfds;
    FD_ZERO(&rfds);

    if (uds_client >= 0)
      FD_SET(uds_client, &rfds);   // Monitor Node socket (UI → Robot)

    FD_SET(uart_fd, &rfds);        // Monitor UART (Robot → UI)

    int maxfd = uart_fd;
    if (uds_client > maxfd) maxfd = uds_client;

    // Timeout = 20ms (keeps system responsive)
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 20000;

    int rc = select(maxfd + 1, &rfds, NULL, NULL, &tv);

    if (rc < 0) {
      if (errno == EINTR) continue;
      perror("select");
      break;
    }

    // =====================================================
    // UI → ROBOT PATH (UDS READ)
    // =====================================================
    if (uds_client >= 0 && FD_ISSET(uds_client, &rfds)) {

      // Read 4-byte length prefix
      uint32_t len_be = 0;
      int r = read_full(uds_client, &len_be, 4);

      // Node disconnected
      if (r == 0) {
        printf("Node disconnected\n");
        close(uds_client);
        uds_client = -1;
        continue;
      }

      if (r > 0) {

        uint32_t len = ntohl(len_be);

        // FIX: Limit JSON size (prevents memory abuse)
        if (len == 0 || len > 4096) {
          close(uds_client);
          uds_client = -1;
          continue;
        }

        char *buf = malloc(len + 1);
        if (!buf) continue;

        int r2 = read_full(uds_client, buf, len);

        if (r2 <= 0) {
          free(buf);
          close(uds_client);
          uds_client = -1;
          continue;
        }

        buf[len] = '\0';

        // -------------------------------
        // Determine if plaintext JSON or encrypted
        // -------------------------------
        if (looks_like_json(buf)) {
          cJSON *probe = cJSON_Parse(buf);
          if (probe) {
            cJSON_Delete(probe);

            // Send instruction to robot
            handle_node_json(uart_fd, uds_client, buf);
          }
        } else {
          // Encrypted path
          handle_encrypted_data(uart_fd, uds_client, buf);
        }

        free(buf);
      }
    }

    // =====================================================
    // ROBOT → UI PATH (UART READ)
    // =====================================================
    if (FD_ISSET(uart_fd, &rfds)) {

      uint8_t tmp[256];
      ssize_t n = read(uart_fd, tmp, sizeof(tmp));

      if (n > 0) {

        // Line buffer for assembling UART messages
        static char uart_line[4096];
        static int uart_idx = 0;

        for (ssize_t i = 0; i < n; i++) {

          char c = tmp[i];

          // -------------------------------
          // End of line → process message
          // -------------------------------
          if (c == '\n') {

            uart_line[uart_idx] = '\0';

            // Debug print (can disable later)
            printf("[UART] %s\n", uart_line);

            // -------------------------------
            // BLE NOTIFICATION HANDLING
            // FIX: Only send to UI if connected
            // -------------------------------
            if (strstr(uart_line, "+NOTIFY:") != NULL) {
              if (uds_client >= 0) {
                parse_notify_and_process(uart_line, uds_client);
              }
            }

            uart_idx = 0;
          }
          else {
            // Build line safely
            if (uart_idx < sizeof(uart_line) - 1) {
              uart_line[uart_idx++] = c;
            } else {
              // FIX: Prevent buffer corruption
              uart_idx = 0;
            }
          }
        }
      }
    }
  }

  // -------------------------------
  // CLEANUP (on exit)
  // -------------------------------
  if (uds_client >= 0) close(uds_client);
  close(uds_listen);
  close(uart_fd);
  unlink(uds_path);

  return 0;
}