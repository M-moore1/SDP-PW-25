#include "ble.h"
#include <stdlib.h>   // calloc, free

/*
 * ble.c
 * -----------------------------------------------------------------------------
 * See ble.h for overview.
 */

static int read_some(int fd, char *buf, size_t cap) {
  if (cap == 0) return 0;
  ssize_t n = read(fd, buf, cap - 1);
  if (n <= 0) return (int)n;
  buf[n] = '\0';
  return (int)n;
}

void msleep(int ms) { usleep(ms * 1000); }

int uart_open_config(const char *dev, speed_t baud) {
  int fd = open(dev, O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (fd < 0) {
    perror("open uart");
    return -1;
  }

  struct termios tio;
  memset(&tio, 0, sizeof(tio));

  if (tcgetattr(fd, &tio) != 0) {
    perror("tcgetattr");
    close(fd);
    return -1;
  }

  cfsetispeed(&tio, baud);
  cfsetospeed(&tio, baud);

  tio.c_cflag = (tio.c_cflag & ~CSIZE) | CS8;
  tio.c_cflag |= (CLOCAL | CREAD);
  tio.c_cflag &= ~(PARENB | PARODD);
  tio.c_cflag &= ~CSTOPB;

  // Default: no HW flow control. If you wire RTS/CTS and want it, set CRTSCTS.
  tio.c_cflag &= ~CRTSCTS;

  tio.c_iflag = IGNPAR;
  tio.c_oflag = 0;
  tio.c_lflag = 0;

  tcflush(fd, TCIFLUSH);
  if (tcsetattr(fd, TCSANOW, &tio) != 0) {
    perror("tcsetattr");
    close(fd);
    return -1;
  }

  return fd;
}

int uart_write_str(int fd, const char *s) {
  ssize_t n = write(fd, s, strlen(s));
  return (n == (ssize_t)strlen(s)) ? 0 : -1;
}

// ------------------------- Command mode helpers -------------------------

int ble_enter_cmd(int uart_fd) {
  char buffer[BLE_READBUF_SZ];

  // Try entering command mode
  if (write(uart_fd, "$$$", 3) != 3) return -1;
  msleep(400);

  memset(buffer, 0, sizeof(buffer));
  int n = read_some(uart_fd, buffer, sizeof(buffer));
  if (n > 0) {
    printf("[PMOD RESPONSE]: %s\r\n", buffer);
    if (strstr(buffer, "CMD")) return 0;
  }

  // Fallback: probe for "?" response, similar to bt2.c pattern
  if (write(uart_fd, "UNKNOWN\r", 8) != 8) return -1;
  msleep(200);

  memset(buffer, 0, sizeof(buffer));
  n = read_some(uart_fd, buffer, sizeof(buffer));
  if (n > 0) {
    printf("[PMOD RESPONSE]: %s\r\n", buffer);
    if (strstr(buffer, "?")) return 1; // got unknown-command indicator
  }

  return -1;
}

int ble_exit_cmd(int uart_fd) {
  char buffer[BLE_READBUF_SZ];

  // Clear pending input, then exit
  tcflush(uart_fd, TCIFLUSH);

  if (write(uart_fd, "---\r", 5) != 5) return -1;
  msleep(400);

  memset(buffer, 0, sizeof(buffer));
  int n = read_some(uart_fd, buffer, sizeof(buffer));
  if (n > 0) {
    printf("[PMOD RESPONSE]: %s\r\n", buffer);
    if (strstr(buffer, "END")) return 0;
  }
  return -1;
}

int ble_connect_mac(int uart_fd, const char *mac) {
  // Per team notes, BLE connect command variants exist:
  //   C,<0-1>,<MAC> where 0=public (never changes), 1=random.
  // We'll default to 0 to match "same behavior" as BT2 connect-by-MAC.
  char cmd[96];
  char buffer[BLE_READBUF_SZ];
  int attempts = 0;

  if (ble_enter_cmd(uart_fd) < 0) return -1;
  tcflush(uart_fd, TCIFLUSH);

  snprintf(cmd, sizeof(cmd), "C,0,%s\r", mac);
  if (write(uart_fd, cmd, strlen(cmd)) != (ssize_t)strlen(cmd)) {
    ble_exit_cmd(uart_fd);
    return -1;
  }

  printf("Connecting (BLE) to %s\r\n", mac);
  fflush(stdout);

  while (attempts < 120) {
    memset(buffer, 0, sizeof(buffer));
    int n = read_some(uart_fd, buffer, sizeof(buffer));
    if (n > 0) {
      printf("\r\n[PMOD RESPONSE]: %s\r\n", buffer);

      if (strstr(buffer, "CONNECT failed") || strstr(buffer, "ERR") || strstr(buffer, "?")) {
        printf("Connection FAILED\r\n");
        ble_exit_cmd(uart_fd);
        return -2;
      }

      if (strstr(buffer, "CONNECT")) {
        printf("\nResult: Connection SUCCESSFUL\r\n");
        // Many modules auto-exit CMD on connect; safe either way.
        return 0;
      }
    }

    msleep(100);
    attempts++;
    if (attempts % 10 == 0) { printf("."); fflush(stdout); }
  }

  printf("Connection Timeout.\r\n");
  ble_exit_cmd(uart_fd);
  return -3;
}

int ble_disconnect(int uart_fd) {
  char buffer[BLE_READBUF_SZ];

  if (ble_enter_cmd(uart_fd) != 0) return -1;
  tcflush(uart_fd, TCIFLUSH);

  uart_write_str(uart_fd, "K,1\r");
  msleep(400);

  memset(buffer, 0, sizeof(buffer));
  int n = read_some(uart_fd, buffer, sizeof(buffer));
  if (n > 0) {
    printf("\n[PMOD RESPONSE]: %s\r\n", buffer);
    if (strstr(buffer, "ERR") || strstr(buffer, "?")) {
      ble_exit_cmd(uart_fd);
      return -2;
    }
  }

  msleep(300);
  ble_exit_cmd(uart_fd);
  return 0;
}

int ble_connect_check(int uart_fd) {
  char buffer[BLE_READBUF_SZ];

  if (ble_enter_cmd(uart_fd) != 0) return -1;
  tcflush(uart_fd, TCIFLUSH);

  uart_write_str(uart_fd, "GK\r");
  msleep(300);

  memset(buffer, 0, sizeof(buffer));
  int n = read_some(uart_fd, buffer, sizeof(buffer));
  if (n > 0) {
    printf("\n[PMOD RESPONSE]: %s\r\n", buffer);

    if (strstr(buffer, "ERR") || strstr(buffer, "?")) {
      ble_exit_cmd(uart_fd);
      return -1;
    }

    // Team notes example:
    //   None - unconnected
    //   <PeerAddr>,<AddrType>,<ConnType> - connected
    // Old bt2.c checks "0,0,0" vs "1,0,0". Keep compatible:
    if (strstr(buffer, "0,0,0")) { ble_exit_cmd(uart_fd); return 1; } // disconnected
    if (strstr(buffer, "1,0,0")) { ble_exit_cmd(uart_fd); return 0; } // connected

    // If it isn't the "0,0,0" style, treat "None" as disconnected
    if (strstr(buffer, "None") || strstr(buffer, "NONE")) { ble_exit_cmd(uart_fd); return 1; }

    // Otherwise assume connected if we got something non-error
    ble_exit_cmd(uart_fd);
    return 0;
  }

  ble_exit_cmd(uart_fd);
  return -1;
}

// ------------------------- GATT helpers -------------------------

int ble_client_init(int uart_fd) {
  // Per notes: "CI - client operation" (call during connection process).
  // We'll just send it; if already in client mode, most firmwares respond OK.
  char buffer[BLE_READBUF_SZ];

  if (ble_enter_cmd(uart_fd) != 0) return -1;
  tcflush(uart_fd, TCIFLUSH);

  uart_write_str(uart_fd, "CI\r");
  msleep(200);

  memset(buffer, 0, sizeof(buffer));
  int n = read_some(uart_fd, buffer, sizeof(buffer));
  if (n > 0) {
    printf("[PMOD RESPONSE]: %s\r\n", buffer);
    if (strstr(buffer, "ERR") || strstr(buffer, "?")) {
      ble_exit_cmd(uart_fd);
      return -1;
    }
  }

  // Some modules remain in CMD mode after CI; leave it to caller to exit if needed.
  ble_exit_cmd(uart_fd);
  return 0;
}

int ble_enable_notifications(int uart_fd) {
  // Per notes: enable notifications by writing 0x0100 to CCCD.
  // They wrote: CHW,002B,0100
  char buffer[BLE_READBUF_SZ];
  if (ble_enter_cmd(uart_fd) != 0) return -1;
  tcflush(uart_fd, TCIFLUSH);

  char cmd[64];
  snprintf(cmd, sizeof(cmd), "CHW,%s,0100\r", BLE_CHAR_CCC);
  if (write(uart_fd, cmd, strlen(cmd)) != (ssize_t)strlen(cmd)) {
    ble_exit_cmd(uart_fd);
    return -1;
  }

  msleep(200);
  memset(buffer, 0, sizeof(buffer));
  int n = read_some(uart_fd, buffer, sizeof(buffer));
  if (n > 0) {
    printf("[PMOD RESPONSE]: %s\r\n", buffer);
    if (strstr(buffer, "ERR") || strstr(buffer, "?")) {
      ble_exit_cmd(uart_fd);
      return -1;
    }
  }

  ble_exit_cmd(uart_fd);
  return 0;
}

int ble_write_bytes(int uart_fd, const char *char_id, const uint8_t *data, size_t len) {
  // CHW expects ASCII hex for data in most firmwares:
  //   CHW,<Characteristic>,<DataHex>\r
  // We'll hex-encode into a temporary buffer.
  if (!char_id || !data) return -1;

  // 2 hex chars per byte + NUL
  size_t hex_len = 2 * len;
  // command framing: "CHW," + id + "," + hex + "\r\0"
  size_t cmd_cap = 4 + 1 + strlen(char_id) + 1 + hex_len + 2 + 1;
  char *cmd = (char*)calloc(1, cmd_cap);
  if (!cmd) return -1;

  char *hex = (char*)calloc(1, hex_len + 1);
  if (!hex) { free(cmd); return -1; }

  hex_encode_lower(data, len, hex);
  snprintf(cmd, cmd_cap, "CHW,%s,%s\r", char_id, hex);

  // Many modules require CMD mode for CHW; we enter/exit around each write for safety.
  // If this is too slow, you can keep CMD mode open externally and write multiple CHW.
  int rc = 0;
  if (ble_enter_cmd(uart_fd) != 0) rc = -1;
  if (rc == 0) {
    tcflush(uart_fd, TCIFLUSH);
    if (write(uart_fd, cmd, strlen(cmd)) != (ssize_t)strlen(cmd)) rc = -1;
    msleep(80);
    // Optional: read/ignore response
    char buffer[BLE_READBUF_SZ];
    memset(buffer, 0, sizeof(buffer));
    int n = read_some(uart_fd, buffer, sizeof(buffer));
    if (n > 0) {
      // Some firmwares respond "AOK" or similar; only fail on clear ERR/'?'
      if (strstr(buffer, "ERR") || strstr(buffer, "?")) rc = -1;
    }
    ble_exit_cmd(uart_fd);
  }

  free(hex);
  free(cmd);
  return rc;
}

int ble_write_packet128(int uart_fd, const uint8_t packet[128]) {
#if BLE_USE_CHUNKING
  // Chunk into small writes: header(4) + payload(BLE_MAX_CHUNK_PAYLOAD)
  static uint8_t seq = 0;
  seq++;

  const size_t payload_per = BLE_MAX_CHUNK_PAYLOAD;
  const size_t total = (128 + payload_per - 1) / payload_per;

  for (size_t idx = 0; idx < total; idx++) {
    uint8_t chunk[4 + BLE_MAX_CHUNK_PAYLOAD];
    memset(chunk, 0, sizeof(chunk));

    chunk[0] = 0x7E;
    chunk[1] = seq;
    chunk[2] = (uint8_t)idx;
    chunk[3] = (uint8_t)total;

    size_t off = idx * payload_per;
    size_t take = payload_per;
    if (off + take > 128) take = 128 - off;

    memcpy(&chunk[4], &packet[off], take);

    if (ble_write_bytes(uart_fd, BLE_CHAR_WRITE, chunk, 4 + take) != 0) {
      return -1;
    }
    msleep(10);
  }
  return 0;
#else
  // Single write of 128 bytes
  return ble_write_bytes(uart_fd, BLE_CHAR_WRITE, packet, 128);
#endif
}

// ------------------------- App packet helpers -------------------------

int uart_send_instruction_ble(int uart_fd, uint64_t instruction) {
  uint8_t packet[128];
  memset(packet, 0, sizeof(packet));
  packet[0] = 'P';

  packet[1] = (uint8_t)((instruction >> 56) & 0xFF);
  packet[2] = (uint8_t)((instruction >> 48) & 0xFF);
  packet[3] = (uint8_t)((instruction >> 40) & 0xFF);
  packet[4] = (uint8_t)((instruction >> 32) & 0xFF);
  packet[5] = (uint8_t)((instruction >> 24) & 0xFF);
  packet[6] = (uint8_t)((instruction >> 16) & 0xFF);
  packet[7] = (uint8_t)((instruction >>  8) & 0xFF);
  packet[8] = (uint8_t)((instruction >>  0) & 0xFF);

  packet[127] = 0x0D;

  // Optional debug
  printf("[TX 128 BYTES / BLE]: ");
  for (int i = 0; i < 128; i++) printf("%02X ", packet[i]);
  printf("\n");

  return ble_write_packet128(uart_fd, packet);
}

int uart_send_encrypted_word_ble(int uart_fd, uint64_t word) {
  uint8_t packet[128];
  memset(packet, 0, sizeof(packet));
  packet[0] = 'E';

  // Pack as 8 bytes big-endian
  uint8_t pt[8];
  for (int i = 0; i < 8; i++) pt[i] = (uint8_t)(word >> (56 - 8*i));

  uint8_t nonce[AES_GCM_NONCE_LEN];
  uint8_t tag[AES_GCM_TAG_LEN];
  uint8_t ct[8];

  if (aes_gcm_random_nonce(nonce) != 0) return -1;
  if (aes_gcm_encrypt_bytes(GS_AES_KEY, nonce, NULL, 0, pt, sizeof(pt), ct, tag) != 0) return -1;

  char blob[2*(AES_GCM_NONCE_LEN + AES_GCM_TAG_LEN + 8) + 1];
  aes_gcm_build_blob_hex(nonce, tag, ct, sizeof(ct), blob);

  size_t blen = strlen(blob);
  if (blen > 126) return -1;
  memcpy(&packet[1], blob, blen);

  packet[127] = 0x0D;

  return ble_write_packet128(uart_fd, packet);
}
