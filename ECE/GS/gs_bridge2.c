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
//   gcc -O2 -Wall -Wextra gs_bridge2.c cJSON.c -o gs_bridge
//   gcc -O2 -Wall -Wextra gs_bridge2.c includes/json_uds/json_uds.c includes/bt2/bt2.c includes/cmd_parser/cmd_parser.c -I./includes/json_uds -I./includes/bt2 -I./includes/cmd_parser -I./includes/cmd_structure -lcjson -o gs_bridge2
//   gcc -O2 -Wall -Wextra \
  gs_bridge2.c \
  includes/json_uds/json_uds.c \
  includes/bt2/bt2.c \
  includes/cmd_parser/cmd_parser.c \
  includes/crypto/aes_gcm_blob.c \
  includes/crypto/gs_key.c \
  ./newcJSON/cJSON.c \
  -I./includes/json_uds \
  -I./includes/bt2 \
  -I./includes/cmd_parser \
  -I./includes/cmd_structure \
  -I./includes/crypto \
  -I./newcJSON \
  -lcrypto \
  -o gs_bridge2
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
#include "includes/bt2/bt2.h"
#include "includes/cmd_parser/cmd_parser.h"
#include "includes/json_uds/json_uds.h"
#include "includes/crypto/aes_gcm_blob.h" // decrpypt json and encrypt for esp32
#include "includes/crypto/gs_key.h"
#include "cJSON.h" // CHANGE                      // cJSON library header (vendored)
// 004B1224B0A6

static int decrypt_blobhex_to_u64_word(const char *blob_hex, uint64_t *out_word);
// ------------------------- Defaults / Config -------------------------

#define DEFAULT_UDS_PATH "/tmp/gs_bridge.sock" // Socket file path for Node<->C IPC
#define DEFAULT_UART_DEV "/dev/ttyPS2"         // Default UART device (Zynq PS UART)
#define DEFAULT_UART_BAUD B115200              // Default baud rate (termios constant)
#define ESP32_MACADDRESS "441d64f17066"

// ------------------------- Bit helpers -------------------------

// Extract nbits starting at bit position lo from a 64-bit word, returning up to 32 bits.
static inline uint32_t get_bits_u32(uint64_t w, int lo, int nbits) {
  return (uint32_t)((w >> lo) & ((1ULL << nbits) - 1ULL)); // shift down and mask
}

// Convert uint64_t to 8 bytes big-endian (network byte order).
static void u64_to_be(uint64_t w, uint8_t out[8]) {
  for (int i = 0; i < 8; i++) {                // For each byte
    out[i] = (uint8_t)(w >> (56 - 8*i));       // Take MSB first, then next, etc.
  }
}

// Convert 8 bytes big-endian to uint64_t.
static uint64_t be_to_u64(const uint8_t in[8]) {
  uint64_t w = 0;                              // Start at 0
  for (int i = 0; i < 8; i++) {                // For each incoming byte
    w = (w << 8) | (uint64_t)in[i];            // Shift left and OR next byte
  }
  return w;                                    // Return reconstructed 64-bit word
}

// Simple XOR checksum over N bytes (cheap v1 integrity check).
static uint8_t xor8(const uint8_t *p, int n) {
  uint8_t x = 0;                               // Start checksum at 0
  for (int i = 0; i < n; i++) x ^= p[i];       // XOR each byte into accumulator
  return x;                                    // Return final XOR
}
// Debug: print bytes in hex (optional but super useful)
static void dump_hex(const char *tag, const uint8_t *b, size_t n) {
  printf("%s (%zu): ", tag, n);
  for (size_t i = 0; i < n; i++) printf("%02X ", b[i]);
  printf("\n");
}

/*
// ------------------------- Unpackers (Robot->Node) -------------------------
// Decode 64-bit words from ESP32 back into values we can send up to Node as JSON.

typedef struct {
  uint8_t speed;                               // Bits 5..11
  uint8_t state;                               // Bit 12 (interpretation TBD)
  uint8_t motor;                               // Bit 13 (interpretation TBD)
  uint8_t robot_id;                            // Bits 14..15
  uint32_t curr_pos;                           // Bits 16..46 (31 bits)
} StatusSR;

// Unpack SR (Status Report) word into struct.
static int unpack_SR(uint64_t w, StatusSR *out) {
  uint8_t type = (uint8_t)get_bits_u32(w, 0, 5);  // Read 5-bit type
  if (type != TYPE_SR) return -1;                 // Not SR -> error

  out->speed = (uint8_t)get_bits_u32(w, 5, 7);    // 7-bit speed
  out->state = (uint8_t)get_bits_u32(w, 12, 1);   // 1-bit state
  out->motor = (uint8_t)get_bits_u32(w, 13, 1);   // 1-bit motor status
  out->robot_id = (uint8_t)get_bits_u32(w, 14, 2);// 2-bit robot id

  // Curr_Pos is 31 bits starting at bit 16.
  out->curr_pos = (uint32_t)((w >> 16) & ((1ULL << 31) - 1ULL));

  return 0;                                       // Success
}

typedef struct {
  uint8_t battery;                              // Bits 5..11
  uint8_t signal;                               // Bits 12..17
  uint8_t security;                             // Bits 18..19
  uint16_t name_id;                             // Bits 20..31 (index into name table)
} HealthHR;

// Unpack HR (Health Report) word into struct.
static int unpack_HR(uint64_t w, HealthHR *out) {
  uint8_t type = (uint8_t)get_bits_u32(w, 0, 5); // Read type
  if (type != TYPE_HR) return -1;                // Not HR -> error

  out->battery  = (uint8_t)get_bits_u32(w, 5, 7);  // battery field
  out->signal   = (uint8_t)get_bits_u32(w, 12, 6); // signal field
  out->security = (uint8_t)get_bits_u32(w, 18, 2); // security field
  out->name_id  = (uint16_t)get_bits_u32(w, 20, 12);// name id index

  return 0;                                      // Success
}
  */


// ------------------------- Robot word -> Node JSON -------------------------
// Convert incoming robot u64 into JSON string and send back to Node via UDS.

static void robot_word_to_node_json(int uds_fd, uint64_t w) {
  if (uds_fd < 0) return;

  uint8_t pl   = (uint8_t)get_bits_u32(w, 0, 2);   // bits 0..1
  uint8_t type = (uint8_t)get_bits_u32(w, 2, 5);   // bits 2..6 (Option X)

  char msg[256];

  switch (type) {

    case ACK_CMD: {
      // Use your packed union/struct to interpret fields
      robot_bt_packet_t p;
      p.raw = w;

      // ack_format_t:
      // pl: 0..1, type:2..6, id:7..17, result_code:18..22, instruction_specific:23..63
      uint16_t id = (uint16_t)p.ack.id;
      uint8_t  rc = (uint8_t)p.ack.result_code;

      snprintf(msg, sizeof(msg),
        "{\"type\":\"ACK\",\"pl\":%u,\"id\":%u,\"result_code\":%u,\"raw_hex\":\"0x%016llX\"}",
        pl, id, rc, (unsigned long long)w);

      uds_send_json(uds_fd, msg);
    } break;

    case HPR_CMD: {
      robot_bt_packet_t p;
      p.raw = w;

      uint8_t alert_type = (uint8_t)p.hpr.alert_type;

      snprintf(msg, sizeof(msg),
        "{\"type\":\"HPR\",\"pl\":%u,\"alert_type\":%u,\"raw_hex\":\"0x%016llX\"}",
        pl, alert_type, (unsigned long long)w);

      uds_send_json(uds_fd, msg);
    } break;

    case STATUS_CMD: {
      // TODO: once you finalize STATUS/SR bit layout, decode fields here.
      // For now, forward raw.
      snprintf(msg, sizeof(msg),
        "{\"type\":\"SR\",\"pl\":%u,\"raw_hex\":\"0x%016llX\"}",
        pl, (unsigned long long)w);
      uds_send_json(uds_fd, msg);
    } break;

    case HEALTH_CMD: {
      // TODO: once you finalize HEALTH/HR bit layout, decode fields here.
      snprintf(msg, sizeof(msg),
        "{\"type\":\"HR\",\"pl\":%u,\"raw_hex\":\"0x%016llX\"}",
        pl, (unsigned long long)w);
      uds_send_json(uds_fd, msg);
    } break;

    default: {
      // Unknown packet type → still forward raw so UI can log it
      snprintf(msg, sizeof(msg),
        "{\"type\":\"RAW\",\"pl\":%u,\"type_code\":%u,\"raw_hex\":\"0x%016llX\"}",
        pl, type, (unsigned long long)w);
      uds_send_json(uds_fd, msg);
    } break;
  }
}
//static void robot_word_to_node_json(int uds_fd, uint64_t w) {}
/*
{
  uint8_t type = (uint8_t)get_bits_u32(w, 0, 5);           // Extract 5-bit type

  if (type == TYPE_SR) {                                   // Status report
    StatusSR sr;                                           // Parsed SR struct
    if (unpack_SR(w, &sr) == 0) {                          // If decode ok
      char msg[256];                                       // Output JSON buffer
      snprintf(msg, sizeof(msg),
        "{\"type\":\"SR\",\"speed\":%u,\"state\":%u,\"motor\":%u,\"robot_id\":%u,\"curr_pos\":%u}",
        sr.speed, sr.state, sr.motor, sr.robot_id, sr.curr_pos);
      uds_send_json(uds_fd, msg);                          // Send JSON to Node
    }
  } else if (type == TYPE_HR) {                            // Health report
    HealthHR hr;                                           // Parsed HR struct
    if (unpack_HR(w, &hr) == 0) {                          // If decode ok
      char msg[256];                                       // Output JSON
      snprintf(msg, sizeof(msg),
        "{\"type\":\"HR\",\"battery\":%u,\"signal\":%u,\"security\":%u,\"name_id\":%u}",
        hr.battery, hr.signal, hr.security, hr.name_id);
      uds_send_json(uds_fd, msg);                          // Send to Node
    }
  } else if (type == TYPE_A) {                             // Ack
    // TODO: once you finalize Ack layout, unpack it here. For now forward raw.
    char msg[128];
    snprintf(msg, sizeof(msg), "{\"type\":\"A\",\"raw_u64\":\"%llu\"}",
             (unsigned long long)w);
    uds_send_json(uds_fd, msg);
  } else if (type == TYPE_HPR) {                           // High priority report
    // TODO: once you finalize HPR layout, unpack it here. For now forward raw.
    char msg[128];
    snprintf(msg, sizeof(msg), "{\"type\":\"HPR\",\"raw_u64\":\"%llu\"}",
             (unsigned long long)w);
    uds_send_json(uds_fd, msg);
  } else {
    // Unknown type -> ignore for now (or forward raw if you want).
  }
}
*/


// Decode a 128-byte UART record into a u64 word (plaintext 'P' only for now).
// Returns 1 if a word was produced, 0 if ignored, -1 on error.
static int uart_record_to_word(const uint8_t rec[128], uint64_t *out_word) {
  if (rec[0] == 'P') {
    uint64_t w = 0;
    for (int i = 0; i < 8; i++) w = (w << 8) | (uint64_t)rec[1 + i];
    *out_word = w;
    return 1;
  }

  if (rec[0] == 'E') {
    // blob lives in rec[1..126], may be NUL-terminated or padded with zeros
    char blob[128];
    size_t n = 0;
    for (size_t i = 1; i < 127 && n + 1 < sizeof(blob); i++) {
      uint8_t c = rec[i];
      if (c == 0) break;
      blob[n++] = (char)c;
    }
    blob[n] = '\0';

    uint64_t w = 0;
    if (decrypt_blobhex_to_u64_word(blob, &w) != 0) return -1;
    *out_word = w;
    return 1;
  }

  // Unknown record type
  return 0;
}

// Send RAW word up to Node for debugging (hex string).
static void send_raw_word_json(int uds_fd, uint64_t w) {
  if (uds_fd < 0) return;
  char msg[128];
  snprintf(msg, sizeof(msg),
           "{\"type\":\"RAW\",\"src\":\"ESP32\",\"u64_hex\":\"0x%016llX\"}",
           (unsigned long long)w);
  uds_send_json(uds_fd, msg);
}


// Return 1 if s looks like JSON (starts with '{' or '[' after whitespace)
static int looks_like_json(const char *s) {
  if (!s) return 0;
  while (*s == ' ' || *s == '\n' || *s == '\r' || *s == '\t') s++;
  return (*s == '{' || *s == '[');
}

static int is_hex_char(char c) {
  return (c >= '0' && c <= '9') ||
         (c >= 'a' && c <= 'f') ||
         (c >= 'A' && c <= 'F');
}

// Return 1 if string is all hex chars (after trimming whitespace) and length >= min_len
static int looks_like_hex_blob(const char *s, size_t min_len) {
  if (!s) return 0;
  while (*s == ' ' || *s == '\n' || *s == '\r' || *s == '\t') s++;
  size_t n = 0;
  for (; s[n] != '\0'; n++) {
    if (s[n] == ' ' || s[n] == '\n' || s[n] == '\r' || s[n] == '\t') continue;
    if (!is_hex_char(s[n])) return 0;
  }
  return (n >= min_len);
}

// Decrypt encrypted hex blob (nonce||tag||ct) into plaintext JSON string.
// Returns 0 on success, -1 on failure.
// NOTE: This is a stub until AES is integrated as a callable function.
static int decrypt_blobhex_to_json(const char *blob_hex, char *out_json, size_t out_cap) {
  uint8_t nonce[AES_GCM_NONCE_LEN];
  uint8_t tag[AES_GCM_TAG_LEN];
  uint8_t ct[4096];
  size_t ct_len = 0;

  if (aes_gcm_parse_blob_hex(blob_hex, nonce, tag, ct, sizeof(ct), &ct_len) != 0) {
    return -1;
  }

  uint8_t pt[4096];
  size_t pt_len = 0;

  // No AAD for now
  int rc = aes_gcm_decrypt_bytes(GS_AES_KEY, nonce, tag, NULL, 0, ct, ct_len, pt, &pt_len);
  if (rc != 0) return -1;

  // Ensure null-terminated and fits
  if (pt_len + 1 > out_cap) return -1;
  memcpy(out_json, pt, pt_len);
  out_json[pt_len] = '\0';
  return 0;
}

static int decrypt_blobhex_to_u64_word(const char *blob_hex, uint64_t *out_word) {
  uint8_t nonce[AES_GCM_NONCE_LEN];
  uint8_t tag[AES_GCM_TAG_LEN];

  uint8_t ct[64];
  size_t ct_len = 0;

  if (aes_gcm_parse_blob_hex(blob_hex, nonce, tag, ct, sizeof(ct), &ct_len) != 0) return -1;
  if (ct_len != 8) return -1; // for word packets we expect exactly 8 bytes ciphertext

  uint8_t pt[16];
  size_t pt_len = 0;

  int rc = aes_gcm_decrypt_bytes(GS_AES_KEY, nonce, tag, NULL, 0, ct, ct_len, pt, &pt_len);
  if (rc != 0) return -1;
  if (pt_len != 8) return -1;

  // Convert 8 bytes big-endian to u64
  uint64_t w = 0;
  for (int i = 0; i < 8; i++) w = (w << 8) | (uint64_t)pt[i];
  *out_word = w;
  return 0;
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


        const char *esp32_mac = ESP32_MACADDRESS;  
        
        printf("Entering cmd\n");
        printf("RN-42: connecting to ESP32 MAC %s...\n", esp32_mac);
        if (rn42_connect_mac(uart_fd, esp32_mac) != 0) {
          printf("RN-42: connect attempt failed (will not retry unless Node reconnects)\n");
        } else {
          printf("RN-42: connect command sent.\n");
        }
        
      }
    }

    fd_set rfds;                                           // Read fd set
    FD_ZERO(&rfds);
    if (uds_client >= 0) FD_SET(uds_client, &rfds);
    FD_SET(uart_fd, &rfds);
    int maxfd = uart_fd;
    if (uds_client > maxfd) maxfd = uds_client;
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

        buf[len] = '\0';

        int handled = 0;

        // Fast path: if it looks like JSON and parses, treat as plaintext JSON
        if (looks_like_json(buf)) {
          cJSON *probe = cJSON_Parse(buf);
          if (probe) {
            cJSON_Delete(probe);
            printf("UDS->C plaintext JSON\n");
            handle_node_json(uart_fd, uds_client, buf);
            handled = 1;
          }
        }

        // If not handled, attempt decrypt path
        if (!handled) {
          // Minimum hex length needed just to contain nonce+tag (no ct): 24+32 = 56
          // Real ct will add more.
          if (!looks_like_hex_blob(buf, 56)) {
            if (uds_client >= 0) uds_send_json(uds_client,
              "{\"type\":\"ERR\",\"msg\":\"UDS payload not JSON and not hex blob\"}");
          } else {
            char plain[4096];
            int drc = decrypt_blobhex_to_json(buf, plain, sizeof(plain));
            if (drc != 0) {
              if (uds_client >= 0) uds_send_json(uds_client,
                "{\"type\":\"ERR\",\"msg\":\"decrypt failed\"}");
            } else {
              // Optional: validate decrypted JSON parses before using
              cJSON *probe2 = cJSON_Parse(plain);
              if (!probe2) {
                if (uds_client >= 0) uds_send_json(uds_client,
                  "{\"type\":\"ERR\",\"msg\":\"decrypted text not valid JSON\"}");
              } else {
                cJSON_Delete(probe2);
                printf("UDS->C decrypted JSON\n");
                handle_node_json(uart_fd, uds_client, plain);
              }
            }
          }
        }

        free(buf);
      }
    }

    // ----- If UART readable: read 128-byte records and forward to Node -----
    if (FD_ISSET(uart_fd, &rfds)) {
      static uint8_t rxbuf[128];
      static size_t rx_used = 0;

      uint8_t tmp[256];
      ssize_t n = read(uart_fd, tmp, sizeof(tmp));
      if (n > 0) {
        size_t off = 0;

        while (off < (size_t)n) {
          size_t space = 128 - rx_used;
          size_t take = ((size_t)n - off < space) ? ((size_t)n - off) : space;

          memcpy(rxbuf + rx_used, tmp + off, take);
          rx_used += take;
          off += take;

          if (rx_used == 128) {
            // We have one full record
            uint64_t w = 0;
            int got = uart_record_to_word(rxbuf, &w);

            if (got == 1) {
              // For now, forward raw word JSON (debug)
              robot_word_to_node_json(uds_client, w);

              // Later step: decode SR/HR/ACK/HPR and call robot_word_to_node_json()
              // robot_word_to_node_json(uds_client, w);
            } else if (got < 0) {
              // Optional: report record decode error
              uds_send_json(uds_client, "{\"type\":\"ERR\",\"msg\":\"uart record decode failed\"}");
            }

            // Reset for next record
            rx_used = 0;
            memset(rxbuf, 0, sizeof(rxbuf));
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