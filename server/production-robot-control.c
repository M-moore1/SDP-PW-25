// Author: Isai Torres
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
//   gcc -O2 -Wall -Wextra gs_bridge.c cJSON.c -o gs_bridge
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

#include "cJSON.h"                      // cJSON library header (vendored)
// 004B1224B0A6
// ------------------------- Defaults / Config -------------------------

#define DEFAULT_UDS_PATH "/tmp/gs_bridge.sock" // Socket file path for Node<->C IPC
#define DEFAULT_UART_DEV "/dev/ttyPS2"         // Default UART device (Zynq PS UART)
#define DEFAULT_UART_BAUD B115200              // Default baud rate (termios constant)

// UART frame constants:
#define UART_PREAMBLE_0 0xAA                   // Frame sync byte 0
#define UART_PREAMBLE_1 0x55                   // Frame sync byte 1
#define UART_PAYLOAD_LEN 8                     // Payload bytes (64-bit word)

// ------------------------- Type mapping (5-bit) -------------------------
// These values match your spec table (type in bits 0..4 of the 64-bit word)

typedef enum {
  TYPE_C   = 0x01,                             // Control
  TYPE_P   = 0x02,                             // Pose
  TYPE_S   = 0x03,                             // System
  TYPE_Q   = 0x04,                             // Query
  TYPE_SR  = 0x05,                             // Status report
  TYPE_HR  = 0x06,                             // Health report
  TYPE_A   = 0x07,                             // Ack
  TYPE_HPR = 0x08                              // High priority report
} MsgType5;

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
// ------------------------- Packers (Node->Robot) -------------------------
// These pack JSON fields into the 64-bit layouts from your spec.

// Pack Control (C):
// bits 0..4  type
// bit 5      W
// bit 6      A
// bit 7      S
// bit 8      d
// bits 9..15 speed (7 bits)
// bits 16..17 priority level (2 bits)
static inline uint64_t pack_C(uint8_t w, uint8_t a, uint8_t s, uint8_t d,
                              uint8_t speed_0_100, uint8_t pl_0_3) {
  uint64_t word = 0;                           // Start empty 64-bit word
  word |= ((uint64_t)(TYPE_C & 0x1F)) << 0;    // Put 5-bit type into bits 0..4
  word |= ((uint64_t)(w & 1)) << 5;            // forward into bit 5
  word |= ((uint64_t)(a & 1)) << 6;            // backward into bit 6
  word |= ((uint64_t)(s & 1)) << 7;            // left into bit 7
  word |= ((uint64_t)(d & 1)) << 8;            // right into bit 8
  word |= ((uint64_t)(speed_0_100 & 0x7F)) << 9;  // speed into bits 9..15 (7 bits)
  word |= ((uint64_t)(pl_0_3 & 0x03)) << 16;  // priority into bits 16..17 (2 bits)
  return word;                                 // Return packed word
}

// Pack Pose (P):
// bits 0..4  type
// bits 5..8  instruction (4-bit opcode, TBD mapping)
// bits 9..10 priority level
// bits 11..22 id (12 bits)
static inline uint64_t pack_P(uint8_t instr_0_15, uint8_t pl_0_3, uint16_t id_0_2047) {
  uint64_t w = 0;                              // Start empty
  w |= ((uint64_t)(TYPE_P & 0x1F)) << 0;       // Type bits
  w |= ((uint64_t)(instr_0_15 & 0x0F)) << 5;   // Instruction nibble
  w |= ((uint64_t)(pl_0_3 & 0x03)) << 9;       // Priority (2 bits)
  w |= ((uint64_t)(id_0_2047 & 0x0FFF)) << 11; // ID (12 bits reserved)
  return w;                                    // Return packed word
}

// Pack System (S):
// bits 0..4   type
// bits 5..8   instruction (4-bit opcode)
// bits 9..18  AC (10 bits)
// bits 19..20 priority
// bits 21..31 id (11 bits)
// bits 32..63 instruction_specific (32 bits)
static inline uint64_t pack_S(uint8_t instr_0_15, uint16_t ac_0_1023, uint8_t pl_0_3,
                              uint16_t id_0_2047, uint32_t instr_spec) {
  uint64_t w = 0;                              // Start empty
  w |= ((uint64_t)(TYPE_S & 0x1F)) << 0;       // Type
  w |= ((uint64_t)(instr_0_15 & 0x0F)) << 5;   // Instruction nibble
  w |= ((uint64_t)(ac_0_1023 & 0x03FF)) << 9;  // AC 10-bit field
  w |= ((uint64_t)(pl_0_3 & 0x03)) << 19;      // Priority 2-bit field
  w |= ((uint64_t)(id_0_2047 & 0x07FF)) << 21; // ID 11-bit field
  w |= ((uint64_t)instr_spec) << 32;           // Instruction-specific 32-bit payload
  return w;                                    // Return packed word
}

// Pack Query (Q):
// bits 0..4   type
// bits 5..8   instruction (what to query, 4-bit)
// bits 9..10  priority
// bits 11..22 id
// bit 23      report_on/off
static inline uint64_t pack_Q(uint8_t instr_0_15, uint8_t pl_0_3, uint16_t id_0_2047, uint8_t report_on) {
  uint64_t w = 0;                              // Start empty
  w |= ((uint64_t)(TYPE_Q & 0x1F)) << 0;       // Type
  w |= ((uint64_t)(instr_0_15 & 0x0F)) << 5;   // Instruction nibble
  w |= ((uint64_t)(pl_0_3 & 0x03)) << 9;       // Priority
  w |= ((uint64_t)(id_0_2047 & 0x0FFF)) << 11; // ID
  w |= ((uint64_t)(report_on & 1)) << 23;      // Report on/off bit
  return w;                                    // Return packed word
}

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

// ------------------------- UART open/config -------------------------
// Open UART device and configure baud/8N1/no flow control (typical SPP serial).

static int uart_open_config(const char *dev, speed_t baud) {
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

// Send one 64-bit word over UART using our frame format.
/*
static int uart_send_word(int uart_fd, uint64_t w) {
  uint8_t payload[8];                                  // 8-byte payload buffer
  u64_to_be(w, payload);                               // Convert u64 to big-endian bytes

  uint8_t buf[2 + 1 + 8 + 1];                          // Total UART frame length = 12 bytes
  buf[0] = UART_PREAMBLE_0;                            // Sync byte 0
  buf[1] = UART_PREAMBLE_1;                            // Sync byte 1
  buf[2] = UART_PAYLOAD_LEN;                           // Payload length (8)
  memcpy(&buf[3], payload, 8);                         // Copy payload
  buf[11] = xor8(payload, 8);                          // Add XOR checksum of payload

  dump_hex("UART TX", buf, sizeof(buf));
  ssize_t n = write(uart_fd, buf, sizeof(buf));        // Write entire frame
  if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return 0; // Nonblocking would-block
  if (n != (ssize_t)sizeof(buf)) return -1;            // Short write -> treat as error for now
  return 0;                                            // Success
}
*/

static int uart_send_word(int uart_fd, uint64_t instruction) {
    // 64 bits + '\r' + '\0' = 66 bytes
    char bit_string[66]; 

    // 1. Convert the uint64_t to an ASCII string of '1's and '0's
    for (int i = 0; i < 64; i++) {
        // We shift 63 down to 0 so the Most Significant Bit is at bit_string[0]
        uint64_t mask = 1ULL << (63 - i);
        bit_string[i] = (instruction & mask) ? '1' : '0';
    }

    // 2. Add the protocol terminator
    bit_string[64] = '\r';
    bit_string[65] = '\0';

    // 3. Write to UART (65 bytes: 64 bits + the carriage return)
    // We do not send the null terminator '\0' over the wire
    ssize_t n = write(uart_fd, bit_string, 65);

    // 4. Error Handling
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0; // Non-blocking: buffer full, try again later
        }
        return -1; // Actual system error
    }

    if (n != 65) {
        return -1; // Short write (connection issue or buffer interrupt)
    }

    // Optional: Log what was sent
    // printf("UART TX: %s\n", bit_string);

    return 0; // Success
}
// ------------------------- RN-42 command helpers -------------------------

static void msleep(int ms) { usleep(ms * 1000); }

static int uart_write_str(int fd, const char *s) {
  ssize_t n = write(fd, s, strlen(s));
  return (n == (ssize_t)strlen(s)) ? 0 : -1;
}

static int rn42_enter_cmd(int uart_fd) {
  // $$$ must be sent with no CR/LF
  if (write(uart_fd, "$$$", 3) != 3) return -1;
  msleep(500);
  return 0;
}

static int rn42_exit_cmd(int uart_fd) {
  if (uart_write_str(uart_fd, "---\r") != 0) return -1;
  msleep(250);
  return 0;
}

static int rn42_connect_mac(int uart_fd, const char *mac) {
  if (rn42_enter_cmd(uart_fd) != 0) return -1;

  char cmd[64];
  // RN-42 connect format is commonly: C,<mac>\r
  snprintf(cmd, sizeof(cmd), "C\r");
  uart_write_str(uart_fd, cmd);

  // Give it time to connect
  msleep(1500);

  rn42_exit_cmd(uart_fd);
  return 0;
}

static int rn42_disconnect(int uart_fd) {
  if (rn42_enter_cmd(uart_fd) != 0) return -1;

  uart_write_str(uart_fd, "K,\r");
  msleep(400);

  rn42_exit_cmd(uart_fd);
  return 0;
}

// ------------------------- UART frame parser -------------------------
// State machine that consumes bytes and outputs full 64-bit payloads.

typedef enum { ST_SYNC0, ST_SYNC1, ST_LEN, ST_PAYLOAD, ST_XOR } UartParseState;

typedef struct {
  UartParseState st;                                   // Current parse state
  uint8_t len;                                         // Payload length (should be 8)
  uint8_t payload[8];                                  // Payload storage
  uint8_t idx;                                         // How many payload bytes read so far
} UartParser;

// Initialize parser state.
static void uart_parser_init(UartParser *p) {
  memset(p, 0, sizeof(*p));                             // Clear everything
  p->st = ST_SYNC0;                                     // Start searching for first preamble byte
}

// Feed one byte into parser. Returns 1 when a complete word is parsed.
static int uart_parser_feed(UartParser *p, uint8_t byte, uint64_t *out_word) {
  switch (p->st) {                                      // Check current state
    case ST_SYNC0:                                      // Looking for 0xAA
      if (byte == UART_PREAMBLE_0) p->st = ST_SYNC1;    // Found first preamble -> next state
      break;

    case ST_SYNC1:                                      // Looking for 0x55
      if (byte == UART_PREAMBLE_1) p->st = ST_LEN;      // Found second preamble -> read len
      else p->st = ST_SYNC0;                            // Not matching -> restart sync
      break;

    case ST_LEN:                                        // Read len byte
      p->len = byte;                                    // Save length
      if (p->len != 8) {                                // If not expected length
        p->st = ST_SYNC0;                               // Restart
        break;
      }
      p->idx = 0;                                       // Reset payload index
      p->st = ST_PAYLOAD;                               // Now read payload bytes
      break;

    case ST_PAYLOAD:                                    // Read 8 payload bytes
      p->payload[p->idx++] = byte;                      // Store next payload byte
      if (p->idx >= 8) p->st = ST_XOR;                  // After 8 bytes, next byte is checksum
      break;

    case ST_XOR: {                                      // Verify checksum
      uint8_t want = xor8(p->payload, 8);               // Compute checksum
      p->st = ST_SYNC0;                                 // Reset for next frame
      if (byte != want) return 0;                       // Bad checksum -> ignore frame
      *out_word = be_to_u64(p->payload);                // Convert payload bytes to u64
      return 1;                                         // Signal: we got a valid word
    }
  }

  return 0;                                             // No full frame yet
}

// ------------------------- UDS framing utilities -------------------------
// Because sockets are byte streams, we send "len + JSON bytes" so receiver knows boundaries.

static int read_full(int fd, void *buf, size_t n) {
  uint8_t *p = (uint8_t*)buf;                           // Byte pointer into buffer
  size_t got = 0;                                       // Bytes read so far
  while (got < n) {                                     // Until we read n bytes
    ssize_t r = read(fd, p + got, n - got);             // Try to read remaining bytes
    if (r == 0) return 0;                               // EOF -> peer disconnected
    if (r < 0) {                                        // Error
      if (errno == EINTR) continue;                     // Interrupted -> retry
      if (errno == EAGAIN || errno == EWOULDBLOCK) return -2; // Nonblocking: no data
      return -1;                                        // Real error
    }
    got += (size_t)r;                                   // Add bytes read
  }
  return 1;                                             // Success
}

static int uds_send_json(int fd, const char *json) {
  uint32_t len = (uint32_t)strlen(json);                // Length of JSON string
  uint32_t len_be = htonl(len);                         // Convert to big-endian length
  if (write(fd, &len_be, 4) != 4) return -1;            // Write 4-byte length
  if (write(fd, json, len) != (ssize_t)len) return -1;  // Write JSON bytes
  return 0;                                             // Success
}

// ------------------------- JSON field helpers -------------------------
// These helpers validate fields exist and are in an allowed range.

static int json_get_u8(const cJSON *obj, const char *key, uint8_t *out, int minv, int maxv) {
  cJSON *it = cJSON_GetObjectItemCaseSensitive((cJSON*)obj, key); // Look up key
  if (!cJSON_IsNumber(it)) return -1;                    // Must be a number
  int v = it->valueint;                                  // Read as int
  if (v < minv || v > maxv) return -2;                   // Range check
  *out = (uint8_t)v;                                     // Output
  return 0;                                              // Success
}

static int json_get_u16(const cJSON *obj, const char *key, uint16_t *out, int minv, int maxv) {
  cJSON *it = cJSON_GetObjectItemCaseSensitive((cJSON*)obj, key); // Look up key
  if (!cJSON_IsNumber(it)) return -1;                    // Must be a number
  int v = it->valueint;                                  // Read as int
  if (v < minv || v > maxv) return -2;                   // Range check
  *out = (uint16_t)v;                                    // Output
  return 0;                                              // Success
}

static int json_get_u32(const cJSON *obj, const char *key, uint32_t *out) {
  cJSON *it = cJSON_GetObjectItemCaseSensitive((cJSON*)obj, key); // Look up key
  if (!cJSON_IsNumber(it)) return -1;                    // Must be number
  double dv = it->valuedouble;                           // Read as double
  if (dv < 0 || dv > 4294967295.0) return -2;            // Range check for u32
  *out = (uint32_t)dv;                                   // Output as u32
  return 0;                                              // Success
}

// ------------------------- Handle Node JSON -------------------------
// Parse incoming JSON from Node and transmit appropriate 64-bit word(s) over UART.

static int handle_node_json(int uart_fd, int uds_fd, const char *json_str) {
  cJSON *root = cJSON_Parse(json_str);                   // Parse JSON text into object
  if (!root) {                                           // Parse failure
    uds_send_json(uds_fd, "{\"type\":\"ERR\",\"msg\":\"bad json\"}"); // Notify Node
    return -1;                                           // Error
  }

  cJSON *type = cJSON_GetObjectItemCaseSensitive(root, "type"); // Get "type" field
  if (!cJSON_IsString(type) || !type->valuestring) {     // Ensure it's a string
    cJSON_Delete(root);                                  // Free parsed JSON object
    uds_send_json(uds_fd, "{\"type\":\"ERR\",\"msg\":\"missing type\"}");
    return -1;                                           // Error
  }

  const char *t = type->valuestring;                     // Convenience pointer to type string

  // ---- Control (C) ----
  if (strcmp(t, "C") == 0) {
    uint8_t f,b,l,r,speed,pl;                            // Fields we’ll extract
    if (json_get_u8(root,"forward",&f,0,1) ||            // Validate each field
        json_get_u8(root,"backward",&b,0,1) ||
        json_get_u8(root,"left",&l,0,1) ||
        json_get_u8(root,"right",&r,0,1) ||
        json_get_u8(root,"speed",&speed,0,100) ||
        json_get_u8(root,"priority_level",&pl,0,3)) {
      uds_send_json(uds_fd, "{\"type\":\"ERR\",\"msg\":\"bad C fields\"}");
      cJSON_Delete(root);
      return -1;
    }

    uint64_t w = pack_C(f,l,b,r,speed,pl);               // Pack fields into 64-bit word
    uart_send_word(uart_fd, w);                          // Send framed binary to ESP32

    // Note: per your spec, no ack for C; status report reflects it later.
  }

  // ---- Pose (P) ----
  else if (strcmp(t, "P") == 0) {
    uint8_t pl;                                          // Priority
    uint16_t id;                                         // ID
    if (json_get_u8(root,"priority_level",&pl,0,3) ||     // Validate
        json_get_u16(root,"id",&id,0,2047)) {
      uds_send_json(uds_fd, "{\"type\":\"ERR\",\"msg\":\"bad P fields\"}");
      cJSON_Delete(root);
      return -1;
    }

    // Temporary spec: accept either "instruction":N or "actions":[N,N,...]
    cJSON *instr = cJSON_GetObjectItemCaseSensitive(root, "instruction");
    cJSON *actions = cJSON_GetObjectItemCaseSensitive(root, "actions");

    if (cJSON_IsNumber(instr)) {                         // Single instruction case
      int op = instr->valueint;                          // Read opcode
      if (op < 0 || op > 15) {                           // Range check 4-bit
        uds_send_json(uds_fd, "{\"type\":\"ERR\",\"msg\":\"P instruction out of range\"}");
      } else {
        uint64_t w = pack_P((uint8_t)op, pl, id);         // Pack pose command
        uart_send_word(uart_fd, w);                       // Send it
      }
    } else if (cJSON_IsArray(actions)) {                 // Multiple instruction list
      int n = cJSON_GetArraySize(actions);               // Number of ops
      for (int i = 0; i < n; i++) {                      // Loop each item
        cJSON *ai = cJSON_GetArrayItem(actions, i);      // Get ith array element
        if (!cJSON_IsNumber(ai)) continue;               // Skip non-numbers
        int op = ai->valueint;                           // Read opcode
        if (op < 0 || op > 15) continue;                 // Skip invalid
        uint64_t w = pack_P((uint8_t)op, pl, id);        // Pack pose
        uart_send_word(uart_fd, w);                      // Send each as separate frame
      }
    } else {
      uds_send_json(uds_fd, "{\"type\":\"ERR\",\"msg\":\"P requires instruction or actions[]\"}");
    }
  }

  // ---- System (S) ----
  else if (strcmp(t, "S") == 0) {
    uint8_t pl, instr;                                   // Priority and instruction
    uint16_t id, ac;                                     // ID and AC
    uint32_t spec;                                       // Instruction-specific payload

    if (json_get_u8(root,"instruction",&instr,0,15) ||    // Validate fields
        json_get_u16(root,"ac",&ac,0,1023) ||
        json_get_u8(root,"priority_level",&pl,0,3) ||
        json_get_u16(root,"id",&id,0,2047) ||
        json_get_u32(root,"instruction_specific",&spec)) {
      uds_send_json(uds_fd, "{\"type\":\"ERR\",\"msg\":\"bad S fields\"}");
      cJSON_Delete(root);
      return -1;
    }

    uint64_t w = pack_S(instr, ac, pl, id, spec);         // Pack system command
    uart_send_word(uart_fd, w);                           // Send over UART
  }

  // ---- Query (Q) ----
  else if (strcmp(t, "Q") == 0) {
    uint8_t pl, instr, report;                            // Priority, instruction, report on/off
    uint16_t id;                                          // ID

    if (json_get_u8(root,"instruction",&instr,0,15) ||     // Validate fields
        json_get_u8(root,"report",&report,0,1) ||
        json_get_u8(root,"priority_level",&pl,0,3) ||
        json_get_u16(root,"id",&id,0,2047)) {
      uds_send_json(uds_fd, "{\"type\":\"ERR\",\"msg\":\"bad Q fields\"}");
      cJSON_Delete(root);
      return -1;
    }

    uint64_t w = pack_Q(instr, pl, id, report);            // Pack query command
    uart_send_word(uart_fd, w);                            // Send over UART
  }

  // ---- Unknown type ----
  else {
    uds_send_json(uds_fd, "{\"type\":\"ERR\",\"msg\":\"unknown type\"}");
  }

  cJSON_Delete(root);                                      // Free JSON object
  return 0;                                                // Success path
}

// ------------------------- Robot word -> Node JSON -------------------------
// Convert incoming robot u64 into JSON string and send back to Node via UDS.

static void robot_word_to_node_json(int uds_fd, uint64_t w) {
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

// ------------------------- UDS server setup -------------------------
// Create a Unix domain socket server that waits for Node to connect.

static int uds_server_listen(const char *path) {
  int fd = socket(AF_UNIX, SOCK_STREAM, 0);                // Create UDS stream socket
  if (fd < 0) { perror("socket uds"); return -1; }         // Error check

  struct sockaddr_un addr;                                 // Address struct for UDS
  memset(&addr, 0, sizeof(addr));                          // Clear it
  addr.sun_family = AF_UNIX;                               // Unix domain socket
  strncpy(addr.sun_path, path, sizeof(addr.sun_path)-1);   // Copy path into struct

  unlink(path);                                            // Remove old socket file if it exists

  if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {// Bind socket file
    perror("bind uds");
    close(fd);
    return -1;
  }

  chmod(path, 0660);                                       // Allow owner/group access (adjust later)

  if (listen(fd, 1) < 0) {                                 // Listen with backlog=1 (single-client)
    perror("listen uds");
    close(fd);
    return -1;
  }

  return fd;                                               // Return listening socket fd
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

  UartParser up;                                           // UART parser state machine
  uart_parser_init(&up);                                   // Initialize parser

  int uds_client = -1;                                     // Node client fd (none yet)
  int bt_connect_attempted = 0;
  while (1) {                                              // Run forever
    if (uds_client < 0) {                                  // If Node not connected
      printf("Waiting for Node connection...\n");          // Tell user
      uds_client = accept(uds_listen, NULL, NULL);         // Accept one client
      if (uds_client < 0) { perror("accept"); continue; }  // Retry on failure
      fcntl(uds_client, F_SETFL, O_NONBLOCK);              // Make Node socket nonblocking
      printf("Node connected.\n");                         // Confirm
      // Connect RN-42 to ESP32 now that Node is connected
      // Try RN-42 connect ONCE after Node connects
      printf("%d", bt_connect_attempted);
      if (!bt_connect_attempted) {
        bt_connect_attempted = 1;

        printf("%d", bt_connect_attempted);

        // Put your ESP32 Bluetooth MAC here (format may need no colons)
        //00:4b:12:24:b0:a6
        const char *esp32_mac = "004B1224B0A6";  // <-- CHANGE THIS
        // 004B1224B0A6 ESP 32 MAC
        // Desktop MAC 001A7DDA7113
        //rn42_enter_cmd(uart_fd);
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
        printf("UDS->C got JSON: %s\n", buf);
        handle_node_json(uart_fd, uds_client, buf);         // Parse + pack + send to UART

        free(buf);                                         // Free buffer
      }
    }

    // ----- If UART readable: read bytes, parse frames, forward to Node -----
    if (FD_ISSET(uart_fd, &rfds)) {                         // UART has data
      uint8_t tmp[256];                                     // Temp read buffer
      ssize_t n = read(uart_fd, tmp, sizeof(tmp));          // Read available bytes
      if (n > 0) {                                          // If bytes received
        for (ssize_t i = 0; i < n; i++) {                   // Feed each byte into parser
          uint64_t word = 0;                                // Output word
          int got = uart_parser_feed(&up, tmp[i], &word);   // Parser state update
          if (got == 1 && uds_client >= 0) {                // If we got a complete frame
            robot_word_to_node_json(uds_client, word);      // Convert to JSON and send to Node
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