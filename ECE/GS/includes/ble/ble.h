#ifndef BLE_H
#define BLE_H

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <stdint.h>
#include <errno.h>
#include <inttypes.h>

#define DEFAULT_UART_DEV "/dev/ttyPS2"         // Default UART device (Zynq PS UART)
#define DEFAULT_UART_BAUD B115200              // Default baud rate (termios constant)

extern volatile int ble_cmd_state;

void msleep(int ms);
int uart_open_config(const char *dev, speed_t baud); 
int ble_uart_write(int uart_fd, const char *str, int size);
int ble_enter_cmd(int uart_fd);
int ble_exit_cmd(int uart_fd);
int ble_reset(int uart_fd);
int ble_enter_client_mode(int uart_fd);
int ble_connect_mac(int uart_fd, const char *mac);
int ble_disconnect(int uart_fd);
int uart_send_str(int uart_fd, char * str, int str_len);

// ADD MAC BUFFER
// RSSI CHECK
int ble_connect_check(int uart_fd);

int ble_init(int uart_fd);


int uart_send_encrypted(int uart_fd, uint8_t *packet);
int uart_send_instruction(int uart_fd, uint64_t instruction);



#endif