#include <fcntl.h>
#include "../conio.c" 
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <stdint.h>
#include <time.h>
#include "../includes/ble/uart_queue.h"
#include "../includes/ble/pmod_esp32.h"

#include "../includes/cmd_structure.h"

#define byte_test_size  156
#define AVG_SAMPLES    100   
#define SEND_INTERVAL  50

//gcc -O2 -Wall -Wextra test_esp32.c ../includes/ble/pmod_esp32.c -I.../includes/cmd_structure ../includes/ble/uart_queue.c -o test_esp.o

uint8_t payload[156] = {
    0x8B, 0xDF, 0x57, 0x3D, 0x3D, 0x50, 0xAF, 0x81, 0xFA, 0xD2, 0x9D, 0x95, 0x5B, 0x91, 0xD6, 0xBC,
    0x09, 0xFF, 0x99, 0x70, 0x95, 0x65, 0xF1, 0x14, 0xAC, 0xDA, 0x91, 0x46, 0x93, 0x79, 0xB8, 0x6E,
    0xC6, 0x38, 0x24, 0xB3, 0x73, 0x39, 0xEE, 0xC7, 0xAA, 0x90, 0x39, 0x29, 0x13, 0x7B, 0xD3, 0x67,
    0xD0, 0x0B, 0x0A, 0x99, 0x53, 0x45, 0x05, 0xFB, 0x3E, 0x60, 0x3E, 0x24, 0xD0, 0x2D, 0x77, 0xB1,
    0xDB, 0xEC, 0xA6, 0x93, 0x1B, 0x98, 0x1D, 0x18, 0xAE, 0x31, 0x6F, 0xA1, 0x4F, 0x2B, 0x32, 0xB8,
    0xCB, 0x77, 0x83, 0x05, 0x17, 0x4D, 0x69, 0x61, 0xFD, 0xD3, 0xBE, 0xA0, 0xBE, 0x07, 0x12, 0x23,
    0xC7, 0x5C, 0x38, 0x56, 0x28, 0x67, 0x88, 0xAA, 0xEA, 0x14, 0xBC, 0xCD, 0x93, 0xA3, 0xF7, 0x6F,
    0xD1, 0xDD, 0xDD, 0x58, 0xC8, 0x24, 0xE3, 0x46, 0x7B, 0x73, 0xFF, 0x81, 0xF4, 0xD4, 0xAF, 0xAD,
    0x61, 0xDB, 0x43, 0x98, 0xF1, 0x19, 0x54, 0x2F, 0x13, 0x6E, 0x2C, 0x6A, 0xAB, 0x3B, 0x68, 0x61,
    0x5B, 0xCE, 0x9D, 0x26, 0xE1, 0x48, 0xF0, 0x42, 0x26, 0xC4, 0x99, 0x4C,
};

int main() {

    int bt_uart = uart_open_config(DEFAULT_UART_DEV, DEFAULT_UART_BAUD); // Opens pmod connections
    set_conio_terminal_mode(); // turns on keyboard reading
    long last = 0;
    printf("Starting ESP32 BLE Test\r\n");
    ble_init(bt_uart);

    uint8_t instruction[8] = { 0x85, 0x90, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 };

    long message_end = 0;
    long message_start = 0;
    long average_time = 0;
    int trial_count = 0;
    int trail_amount = 0;

    while(1){
        ble_uart_check(bt_uart);
        char rx_buffer[1024]; 

        if (uart_queue_pop(&uart_queue, rx_buffer) == 0){
            //printf("[UART OUTPUT] %s\r\n", rx_buffer);
            if (strstr(rx_buffer, "NOTIFY") != NULL) 
            {
                
                message_end = (clock() / (CLOCKS_PER_SEC / 1000000));
                long latency = message_end - message_start;
                
                average_time += latency;
                trial_count++;
                

                if (trial_count >= AVG_SAMPLES) {
                    long avg = average_time / trial_count;
                    printf("\r\n=== STATS #%d [SEND RATE %d][%d Samples] ===\r\n", trail_amount, SEND_INTERVAL, trial_count);
                    printf("Average Latency: %.3f ms\r\n", avg / 1000.0);
                    printf("==========================\r\n\n");
                    
                    // Reset variables for the next batch
                    average_time = 0;
                    trial_count = 0;
                    trail_amount++;
                } else {
                    // Optional: print individual latency and progress
                    printf("Trial %d/%d - Latency: %.3f ms\r", trial_count, AVG_SAMPLES, latency / 1000.0);
                    fflush(stdout);
                }
            }
        }

        if(kbhit()){
            char c = getch();
            if (c == 'q')break;
            if(c == 'c'){
                //read pmod 0 and print 
                uint32_t reg_output = gpio_read(PMOD_0_RST);
                printf("[Reg Output] %u\r\n", reg_output);
            }
            if(c == 'v'){
                int x = pmod_esp32_reset(bt_uart);
                printf("%d\r\n", x);
            }
            if (c == 'z') {
                if (ble_connect(bt_uart, NULL) == 0) {
                    printf("Connected \r\n");
                } else {
                    printf("Failed to connect\r\n");
                }
            }
            if (c == 'x') {
                if (ble_discon(bt_uart) == 0) {
                    printf("Disconnect \r\n");
                } else {
                    printf("Failed to disconnect\r\n");
                }
            }
            if (c == 'b') {
                char conn_results[128]; 
                memset(conn_results, 0, sizeof(conn_results));

                if (pmod_name(bt_uart, "ROBOT_ESP32", conn_results) == 0) {
                    printf("Current Params: %s\r\n", conn_results);
                } else {
                    printf("Failed to retrieve parameters. (Are you connected?)\r\n");
                }
            }

            if (c == 'a') {
                //char buf[1024] = {0};
                
                // Enable notifications: write 0x0001 to CCCD (srv=3, char=1, desc=1, len=2)
                //ble_write(bt_uart, 3, 1, -1, <str>, <str_len>);
                //uint8_t test[] = {0x12, 0x13};

                //if (send_at_cmd(bt_uart, "AT+BLEGATTCINCLSRV=0,3\r\n", "+BLEGATTCINCLSRV:", buf, 10000) < 0) return -1;
                //printf("RESPONSE ")
                
            }
            
        }
        
        long now = clock() / (CLOCKS_PER_SEC / 1000000);
        if (now - last >= SEND_INTERVAL * 1000) {
            message_start = now;
            ble_send_instruction(bt_uart, instruction);
            //ble_send_pkt(bt_uart, payload, 156);
            last = now;
        }
        
        
    }

    close(bt_uart);
    return 0;
}
