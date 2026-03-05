// gs_bridgeble.c
// -----------------------------------------------------------------------------
// Bridge daemon (BLE version):
//   Node.js <-> Unix Domain Socket (JSON, length-prefixed) <-> C bridge
//   C bridge <-> UART (BLE PMOD, GATT writes + notifications) <-> ESP32
//
// NOTE ON TRANSPORT:
//   - TX path: uses BLE characteristic writes via ble.c (CHW,002A,...), keeping your
//     existing 128-byte application packet format ('P'/'E' records).
//   - RX path: THIS FILE STILL COLLECTS RAW BYTES INTO 128-byte RECORDS, SAME AS
//     THE BT2 VERSION. This will work ONLY if your BLE module forwards notify data
//     back as raw bytes. If your module prints notify events as ASCII like
//     "NTF,002A,<hex...>", you will need a small parser to convert that to bytes.

// -----------------------------------------------------------------------------

#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <termios.h>
#include <unistd.h>

#include "includes/cmd_structure.h"
#include "includes/ble/ble.h"                  // BLE replacement for BT2/RN-42
#include "includes/cmd_parser_ble/cmd_parser_ble.h"
#include "includes/json_uds/json_uds.h"
#include "includes/crypto/aes_gcm_blob.h"
#include "includes/crypto/gs_key.h"
#include "cJSON.h"

static int decrypt_blobhex_to_u64_word(const char *blob_hex, uint64_t *out_word);

// ------------------------- Defaults / Config -------------------------

#define DEFAULT_UDS_PATH "/tmp/gs_bridge.sock"
#define DEFAULT_UART_DEV "/dev/ttyPS2"
#define DEFAULT_UART_BAUD B115200
#define ESP32_MACADDRESS "441d64f17066"

// ------------------------- Bit helpers -------------------------

static inline uint32_t get_bits_u32(uint64_t w, int lo, int nbits) {
  return (uint32_t)((w >> lo) & ((1ULL << nbits) - 1ULL));
}

static uint64_t be_to_u64(const uint8_t in[8]) {
  uint64_t w = 0;
  for (int i = 0; i < 8; i++) w = (w << 8) | (uint64_t)in[i];
  return w;
}

// ------------------------- Robot word -> Node JSON -------------------------

static void robot_word_to_node_json(int uds_fd, uint64_t w) {
  if (uds_fd < 0) return;

  uint8_t pl   = (uint8_t)get_bits_u32(w, 0, 2);
  uint8_t type = (uint8_t)get_bits_u32(w, 2, 5);

  char msg[256];

  switch (type) {
    case ACK_CMD: {
      robot_bt_packet_t p; p.raw = w;
      uint16_t id = (uint16_t)p.ack.id;
      uint8_t  rc = (uint8_t)p.ack.result_code;

      snprintf(msg, sizeof(msg),
        "{\"type\":\"ACK\",\"pl\":%u,\"id\":%u,\"result_code\":%u,\"raw_hex\":\"0x%016llX\"}",
        pl, id, rc, (unsigned long long)w);
      uds_send_json(uds_fd, msg);
    } break;

    case HPR_CMD: {
      robot_bt_packet_t p; p.raw = w;
      uint8_t alert_type = (uint8_t)p.hpr.alert_type;

      snprintf(msg, sizeof(msg),
        "{\"type\":\"HPR\",\"pl\":%u,\"alert_type\":%u,\"raw_hex\":\"0x%016llX\"}",
        pl, alert_type, (unsigned long long)w);
      uds_send_json(uds_fd, msg);
    } break;

    case STATUS_CMD: {
      snprintf(msg, sizeof(msg),
        "{\"type\":\"SR\",\"pl\":%u,\"raw_hex\":\"0x%016llX\"}",
        pl, (unsigned long long)w);
      uds_send_json(uds_fd, msg);
    } break;

    case HEALTH_CMD: {
      snprintf(msg, sizeof(msg),
        "{\"type\":\"HR\",\"pl\":%u,\"raw_hex\":\"0x%016llX\"}",
        pl, (unsigned long long)w);
      uds_send_json(uds_fd, msg);
    } break;

    default: {
      snprintf(msg, sizeof(msg),
        "{\"type\":\"RAW\",\"pl\":%u,\"type_code\":%u,\"raw_hex\":\"0x%016llX\"}",
        pl, type, (unsigned long long)w);
      uds_send_json(uds_fd, msg);
    } break;
  }
}

// Decode a 128-byte record into a u64 word ('P' plaintext or 'E' encrypted blob)
static int uart_record_to_word(const uint8_t rec[128], uint64_t *out_word) {
  if (rec[0] == 'P') {
    uint64_t w = 0;
    for (int i = 0; i < 8; i++) w = (w << 8) | (uint64_t)rec[1 + i];
    *out_word = w;
    return 1;
  }

  if (rec[0] == 'E') {
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

  return 0;
}

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

static int decrypt_blobhex_to_json(const char *blob_hex, char *out_json, size_t out_cap) {
  uint8_t nonce[AES_GCM_NONCE_LEN];
  uint8_t tag[AES_GCM_TAG_LEN];
  uint8_t ct[4096];
  size_t ct_len = 0;

  if (aes_gcm_parse_blob_hex(blob_hex, nonce, tag, ct, sizeof(ct), &ct_len) != 0) return -1;

  uint8_t pt[4096];
  size_t pt_len = 0;

  int rc = aes_gcm_decrypt_bytes(GS_AES_KEY, nonce, tag, NULL, 0, ct, ct_len, pt, &pt_len);
  if (rc != 0) return -1;

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
  if (ct_len != 8) return -1;

  uint8_t pt[16];
  size_t pt_len = 0;

  int rc = aes_gcm_decrypt_bytes(GS_AES_KEY, nonce, tag, NULL, 0, ct, ct_len, pt, &pt_len);
  if (rc != 0) return -1;
  if (pt_len != 8) return -1;

  uint64_t w = 0;
  for (int i = 0; i < 8; i++) w = (w << 8) | (uint64_t)pt[i];
  *out_word = w;
  return 0;
}

// ------------------------- Main -------------------------

int main(int argc, char **argv) {
  const char *uart_dev = DEFAULT_UART_DEV;
  const char *env_uart = getenv("UART_DEV");
  if (env_uart && env_uart[0]) uart_dev = env_uart;
  if (argc >= 2) uart_dev = argv[1];

  const char *uds_path = DEFAULT_UDS_PATH;

  int uart_fd = uart_open_config(uart_dev, DEFAULT_UART_BAUD);
  if (uart_fd < 0) return 1;

  int uds_listen = uds_server_listen(uds_path);
  if (uds_listen < 0) return 1;

  printf("Bridge up (BLE). UDS=%s UART=%s\n", uds_path, uart_dev);

  int uds_client = -1;
  int ble_connect_attempted = 0;

  while (1) {
    if (uds_client < 0) {
      printf("Waiting for Node connection...\n");
      uds_client = accept(uds_listen, NULL, NULL);
      if (uds_client < 0) { perror("accept"); continue; }
      fcntl(uds_client, F_SETFL, O_NONBLOCK);
      printf("Node connected.\n");

      // Connect BLE PMOD to ESP32 ONCE after Node connects
      if (!ble_connect_attempted) {
        ble_connect_attempted = 1;

        const char *esp32_mac = ESP32_MACADDRESS;
        printf("BLE: connecting to ESP32 MAC %s...\n", esp32_mac);

        if (ble_connect_mac(uart_fd, esp32_mac) != 0) {
          printf("BLE: connect attempt failed (will not retry unless Node reconnects)\n");
        } else {
          printf("BLE: connected.\n");

          // Recommended: initialize client mode + enable notifications
          if (ble_client_init(uart_fd) != 0) {
            printf("BLE: warning: client init failed\n");
          }
          if (ble_enable_notifications(uart_fd) != 0) {
            printf("BLE: warning: enable notifications failed\n");
          }
        }
      }
    }

    fd_set rfds;
    FD_ZERO(&rfds);
    if (uds_client >= 0) FD_SET(uds_client, &rfds);
    FD_SET(uart_fd, &rfds);

    int maxfd = uart_fd;
    if (uds_client > maxfd) maxfd = uds_client;

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 20000;

    int rc = select(maxfd + 1, &rfds, NULL, NULL, &tv);
    if (rc < 0) {
      if (errno == EINTR) continue;
      perror("select");
      break;
    }

    // ----- If Node socket readable: read 1 framed JSON message -----
    if (uds_client >= 0 && FD_ISSET(uds_client, &rfds)) {
      uint32_t len_be = 0;
      int r = read_full(uds_client, &len_be, 4);
      if (r == 0) {
        close(uds_client);
        uds_client = -1;
        ble_connect_attempted = 0; // so we reconnect next time Node connects
        continue;
      }

      if (r > 0) {
        uint32_t len = ntohl(len_be);
        if (len == 0 || len > (1024*1024)) {
          close(uds_client);
          uds_client = -1;
          ble_connect_attempted = 0;
          continue;
        }

        char *buf = (char*)malloc(len + 1);
        if (!buf) {
          close(uds_client);
          uds_client = -1;
          ble_connect_attempted = 0;
          continue;
        }

        int r2 = read_full(uds_client, buf, len);
        if (r2 <= 0) {
          free(buf);
          close(uds_client);
          uds_client = -1;
          ble_connect_attempted = 0;
          continue;
        }

        buf[len] = '\0';
        int handled = 0;

        if (looks_like_json(buf)) {
          cJSON *probe = cJSON_Parse(buf);
          if (probe) {
            cJSON_Delete(probe);
            printf("UDS->C plaintext JSON\n");
            handle_node_json(uart_fd, uds_client, buf);
            handled = 1;
          }
        }

        if (!handled) {
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
            uint64_t w = 0;
            int got = uart_record_to_word(rxbuf, &w);

            if (got == 1) {
              robot_word_to_node_json(uds_client, w);
            } else if (got < 0) {
              if (uds_client >= 0) uds_send_json(uds_client,
                "{\"type\":\"ERR\",\"msg\":\"uart record decode failed\"}");
            }

            rx_used = 0;
            memset(rxbuf, 0, sizeof(rxbuf));
          }
        }
      }
    }
  }

  if (uds_client >= 0) close(uds_client);
  close(uds_listen);
  close(uart_fd);
  unlink(uds_path);

  return 0;
}
