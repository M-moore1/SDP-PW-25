#ifndef BLE_H
#define BLE_H

/*
 * ble.h
 * -----------------------------------------------------------------------------
 * BLE PMOD UART driver (RN-?? BLE variant style AT commands, per team notes)
 *
 * Goal: provide the SAME high-level API shape as bt2.h/bt2.c (RN-42 SPP),
 * but implemented on top of BLE GATT operations (writes + notifications).
 *
 * Transport model:
 *   - Commands are issued over UART to the BLE module.
 *   - Application payloads are sent via characteristic WRITE commands (CHW,...).
 *   - RX data comes back from the module as UART text responses / notification events.
 *
 * IMPORTANT:
 *   BLE is not a raw byte stream like SPP. 128-byte packets may need chunking
 *   depending on MTU/module limits. This library includes an OPTIONAL chunking
 *   layer that wraps your 128-byte packet into multiple CHW writes.
 *
 * Chunking format (default, if BLE_USE_CHUNKING=1):
 *   Each BLE "chunk" write sends:
 *     [0]  0x7E              (magic)
 *     [1]  seq               (sequence number increments per 128B packet)
 *     [2]  idx               (0..total-1)
 *     [3]  total             (number of chunks)
 *     [4..] payload bytes
 *
 * The ESP32 side must reassemble (seq, total, idx) and recover the exact 128 bytes.
 *
 * If your negotiated MTU supports >= 132 bytes of write payload, you can disable
 * chunking by setting BLE_USE_CHUNKING=0 and just write 128 bytes in one go.
 * -----------------------------------------------------------------------------
 */

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <stdint.h>
#include <errno.h>
#include <inttypes.h>

#include "aes_gcm_blob.h"   // for AES_GCM_* constants and helpers
#include "gs_key.h"         // GS_AES_KEY

#define DEFAULT_UART_DEV "/dev/ttyPS2"
#define DEFAULT_UART_BAUD B115200

// ------------------------- Tunables -------------------------

// Characteristic IDs (from team notes / BLE service design)
#ifndef BLE_CHAR_WRITE
#define BLE_CHAR_WRITE "002A"   // RW characteristic (ZCU102 -> ESP32)
#endif

#ifndef BLE_CHAR_CCC
#define BLE_CHAR_CCC   "002B"   // CCCD / notify-enable characteristic (enable notifications)
#endif

// Enable chunking by default (safe across small MTUs)
#ifndef BLE_USE_CHUNKING
#define BLE_USE_CHUNKING 1
#endif

// Max bytes that we attempt to place into one CHW write *payload* (excluding CHW text framing).
// For small MTUs, 20 bytes is typical. If your module supports larger writes, you can increase.
#ifndef BLE_MAX_CHUNK_PAYLOAD
#define BLE_MAX_CHUNK_PAYLOAD 16   // 16 payload bytes + 4 header = 20 total bytes per chunk
#endif

// Max line we will read from module (UART text responses/events)
#ifndef BLE_READBUF_SZ
#define BLE_READBUF_SZ 512
#endif

// -----------------------------------------------------------------
// UART helpers (same as bt2.* so you can swap includes with minimal changes)
// -----------------------------------------------------------------
void msleep(int ms);
int  uart_open_config(const char *dev, speed_t baud);
int  uart_write_str(int fd, const char *s);

// -----------------------------------------------------------------
// Module command mode + connection management
// -----------------------------------------------------------------
int ble_enter_cmd(int uart_fd);                 // "$$$" -> expect "CMD"
int ble_exit_cmd(int uart_fd);                  // "---\r" -> expect "END"
int ble_connect_mac(int uart_fd, const char *mac);  // "C,0,<mac>\r" (addr type 0 by default)
int ble_disconnect(int uart_fd);                // "K,1\r"
int ble_connect_check(int uart_fd);             // "GK\r" -> 0 connected, 1 disconnected, -1 error

// -----------------------------------------------------------------
// BLE GATT helpers
// -----------------------------------------------------------------
int ble_client_init(int uart_fd);               // "CI\r" (client operation), safe to call even if already in client mode
int ble_enable_notifications(int uart_fd);      // writes 0x0100 to CCC to enable notify
int ble_write_bytes(int uart_fd, const char *char_id, const uint8_t *data, size_t len);

// Convenience: write exactly one 128-byte application packet using BLE_CHAR_WRITE,
// optionally chunked depending on BLE_USE_CHUNKING.
int ble_write_packet128(int uart_fd, const uint8_t packet[128]);

// -----------------------------------------------------------------
// Your app-level sends (same as bt2.*), but transported over BLE GATT write.
// These keep your current 128-byte packet format:
//   - plaintext marker 'P' + 8B instruction at [1..8]
//   - encrypted marker 'E' + hex blob at [1..] (nonce||tag||ct in hex)
// -----------------------------------------------------------------
int uart_send_instruction_ble(int uart_fd, uint64_t instruction);
int uart_send_encrypted_word_ble(int uart_fd, uint64_t word);

#endif
