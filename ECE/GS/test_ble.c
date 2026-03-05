// test_ble.c
// -----------------------------------------------------------------------------
// BLE PMOD test harness (similar to BT2 test_2.c)
// - Opens UART to BLE PMOD
// - Keyboard controls connect/disconnect/link-check
// - Sends 64-bit CONTROL/System packets as 128-byte app records over BLE
//
// Build:
//   gcc -O2 -Wall -Wextra test_ble.c includes/ble/ble.c \
  includes/crypto/aes_gcm_blob.c includes/crypto/gs_key.c \
     -I./includes/ble -I./includes/cmd_structure -I./includes/crypto \
     -lcrypto -o test_ble
//
// Run (example):
//   ./test_ble
//
// Notes:
// - RX behavior depends on BLE module firmware:
//   If notifications come back as RAW BYTES, you'll see binary data in read().
//   If they come back as ASCII "NTF,....", you'll see readable text.
// -----------------------------------------------------------------------------

#include <fcntl.h>
#include "conio.c"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <stdint.h>
#include <time.h>

#include "includes/ble/ble.h"
#include "includes/cmd_structure.h"

#define ESP32_MAC "441d64f17066"

static void print_rx(const uint8_t *buf, ssize_t n) {
  printf("[RX %zd]: ", n);
  for (ssize_t i = 0; i < n; i++) {
    uint8_t c = buf[i];
    // print ascii if printable, else hex
    if (c >= 32 && c <= 126) printf("%c", (char)c);
    else printf("\\x%02X", c);
  }
  printf("\r\n");

  // also hex dump (sometimes nicer)
  printf("[RX HEX]: ");
  for (ssize_t i = 0; i < n; i++) printf("%02X ", buf[i]);
  printf("\r\n");
  fflush(stdout);
}

int main(void) {
  int ble_uart = uart_open_config(DEFAULT_UART_DEV, DEFAULT_UART_BAUD);
  if (ble_uart < 0) return 1;

  set_conio_terminal_mode();

  long last = 0;

  int speed = 50;
  int connected = 0;

  int send_security_update = 0;
  int send = 0;

  robot_bt_packet_t ctrl_inst = (robot_bt_packet_t){0};
  robot_bt_packet_t sys_inst  = (robot_bt_packet_t){0};

  // Example: your system packet template
  sys_inst.sys.pl = 1;
  sys_inst.sys.ac = 0x3FF;
  sys_inst.sys.type = System_CMD;

  printf("BLE test running.\n");
  printf("Keys:\n");
  printf("  q = quit\n");
  printf("  c = enter CMD mode\n");
  printf("  v = exit CMD mode\n");
  printf("  z = connect to ESP32 MAC\n");
  printf("  x = disconnect\n");
  printf("  b = link check (GK)\n");
  printf("  n = client init (CI)\n");
  printf("  m = enable notifications (CHW CCCD)\n");
  printf("  w/a/s/d = set control bits\n");
  printf("  +/- = speed up/down\n");
  printf("  1 = security ON (system cmd)\n");
  printf("  2 = security OFF (system cmd)\n");
  printf("  e = send encrypted word test (ENC)\n");
  printf("\n");

  while (1) {
    uint8_t rx_buffer[256];
    ssize_t bytes_read = read(ble_uart, rx_buffer, sizeof(rx_buffer));
    if (bytes_read > 0) {
      print_rx(rx_buffer, bytes_read);
    }

    if (kbhit()) {
      char c = getch();
      if (c == 'q') break;

      if (c == 'w') ctrl_inst.ctrl.w = 1;
      if (c == 'a') ctrl_inst.ctrl.a = 1;
      if (c == 's') ctrl_inst.ctrl.s = 1;
      if (c == 'd') ctrl_inst.ctrl.d = 1;
      if (c == '+') { if (speed < 100) speed += 1; }
      if (c == '-') { if (speed > 0) speed -= 1; }

      if (c == 'c') {
        printf("Entering CMD MODE...\r\n");
        int r = ble_enter_cmd(ble_uart);
        if (r == 0) printf("CMD Mode entered OK\r\n");
        else if (r == 1) printf("CMD Mode maybe already active (got '?')\r\n");
        else printf("Failed to enter CMD mode\r\n");
      }

      if (c == 'v') {
        printf("Exiting CMD MODE...\r\n");
        int r = ble_exit_cmd(ble_uart);
        if (r == 0) printf("Exited CMD mode OK\r\n");
        else printf("Failed to exit CMD mode\r\n");
      }

      if (c == 'z') {
        printf("Connecting BLE to ESP32 MAC %s...\r\n", ESP32_MAC);
        int r = ble_connect_mac(ble_uart, ESP32_MAC);
        if (r == 0) {
          printf("Connect success\r\n");
          connected = 1;
        } else {
          printf("Connect failed (%d)\r\n", r);
        }
      }

      if (c == 'x') {
        printf("Disconnecting...\r\n");
        int r = ble_disconnect(ble_uart);
        if (r == 0) {
          printf("Disconnect success\r\n");
          connected = 0;
        } else {
          printf("Disconnect failed (%d)\r\n", r);
        }
      }

      if (c == 'b') {
        int r = ble_connect_check(ble_uart);
        if (r == 0) {
          printf("Device Connected\r\n");
          connected = 1;
        } else if (r == 1) {
          printf("Device NOT connected\r\n");
          connected = 0;
        } else {
          printf("Link check error (%d)\r\n", r);
        }
      }

      if (c == 'n') {
        printf("Sending CI (client init)...\r\n");
        int r = ble_client_init(ble_uart);
        printf("CI result = %d\r\n", r);
      }

      if (c == 'm') {
        printf("Enabling notifications (CCCD write)...\r\n");
        int r = ble_enable_notifications(ble_uart);
        printf("Notify enable result = %d\r\n", r);
      }

      if (c == '1') {
        send_security_update = 1;
        sys_inst.sys.instruction = SECURITY_LEVEL;
        sys_inst.sys.id = 0x05;
        sys_inst.sys.specific = 0x01;
        printf("Setting Security ON\r\n");
      }

      if (c == '2') {
        send_security_update = 1;
        sys_inst.sys.instruction = SECURITY_LEVEL;
        sys_inst.sys.id = 0x05;
        sys_inst.sys.specific = 0x00;
        printf("Setting Security OFF\r\n");
      }

      if (c == 'e') {
        // Quick encrypted-word test (arbitrary)
        uint64_t test_word = 0x1122334455667788ULL;
        printf("Sending encrypted test word: 0x%016llX\r\n",
               (unsigned long long)test_word);

        int r = uart_send_encrypted_word_ble(ble_uart, test_word);
        printf("ENC send result = %d\r\n", r);
      }

      // any keypress that sets direction or security triggers a send on next tick
      send = 1;
    }

    long now = clock() / (CLOCKS_PER_SEC / 1000);
    if (now - last >= 60) {
      if (connected == 1 && send == 1) {
        // Build CONTROL instruction packet (same struct as your BT2 test)
        ctrl_inst.ctrl.pl = 1;
        ctrl_inst.ctrl.type = CONTROL_CMD;
        ctrl_inst.ctrl.speed = speed;

        // BLE TX (sends 128-byte app packet via characteristic write)
        int r = uart_send_instruction_ble(ble_uart, ctrl_inst.raw);
        printf("CTRL send result=%d speed=%d raw=0x%016llX\r\n",
               r, speed, (unsigned long long)ctrl_inst.raw);

        // clear control bits for next press
        ctrl_inst.raw = 0;

        if (send_security_update) {
          int r2 = uart_send_instruction_ble(ble_uart, sys_inst.raw);
          printf("SYS send result=%d raw=0x%016llX\r\n",
                 r2, (unsigned long long)sys_inst.raw);
          send_security_update = 0;
        }

        send = 0;
      }
      last = now;
    }
  }

  close(ble_uart);
  return 0;
}