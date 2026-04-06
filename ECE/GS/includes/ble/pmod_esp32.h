#ifndef PMOD_EPSP32_H
#define PMOD_EPSP32_H

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <stdint.h>
#include <errno.h>
#include <inttypes.h>
#include <time.h>

#define DEFAULT_UART_DEV "/dev/ttyPS2"         // Default UART device (Zynq PS UART)
#define DEFAULT_UART_BAUD B115200  
#define PMOD_0_GPIO_0 516 // ESP GPIO #1
#define PMOD_0_RST 517 // ESP RST
#define PMOD_0_MODE 518 // ESP Mode
#define PMOD_0_GPIO_1 519 // ESP GPIO #2

#define PMOD_1_GPIO_0 520
#define PMOD_1_GPIO_1 521
#define PMOD_1_GPIO_2 522
#define PMOD_1_GPIO_3 523

#define PMOD_DEV_NAME "SDP_2635_GS"
#define ESP32_MAC "44:1d:64:f1:1a:86"
#define CONN_IDX 0

#define PAYLOAD_BYTES    156
#define PAYLOAD_HEX_LEN  (PAYLOAD_BYTES * 2)  
#define PACKET_BYTES     160                   
#define PACKET_HEX_LEN   (PACKET_BYTES * 2)  

extern volatile int BLE_CONNECTED;

// UART and GPIO Functions
int uart_open_config(const char *dev, speed_t baud);
int gpio_export(int gpio_num);
int gpio_set_direction(int gpio_num, const char *dir);
int gpio_write(int gpio_num, uint32_t value);
uint32_t gpio_read(int gpio_num);

// PMOD ESP32 Function
int send_at_cmd(int uart_fd, const char *cmd, const char *prefix, char *out_value, int timeout_ms);
int ble_write(int uart_fd, int srv, int chr, int desc, uint8_t *data, int len);
int pmod_esp32_reset(int uart_fd);
int pmod_esp32_init(int uart_fd);

// BLE Functions

int get_pmod_mac(int uart_fd, char *MAC_Output);
int pmod_name(int uart_fd, const char *set_name, char *out_name); 

int ble_init(int uart_fd);
int ble_discon(int uart_fd);
int ble_notification(int uart_fd, int enable);
int ble_connect(int uart_fd, const char *MAC);
int get_ble_conn_params(int uart_fd, char *params_out);

int ble_send_pkt(int uart_fd, uint8_t *data, int data_len);
int ble_send_instruction(int uart_fd, uint8_t instruction[8]);
uint64_t get_now_ms();

#endif