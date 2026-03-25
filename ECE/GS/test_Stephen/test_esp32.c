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
#define SEND_INTERVAL  1000

//gcc -O2 -Wall -Wextra test_esp32.c ../includes/ble/pmod_esp32.c -I.../includes/cmd_structure ../includes/ble/uart_queue.c -o test_esp.o

uint8_t payload[156] = {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, // Starts at 1, counts up 8 bytes
    0x00 // All remaining 148 elements are automatically set to 0
};

int main() {

    int bt_uart = uart_open_config(DEFAULT_UART_DEV, DEFAULT_UART_BAUD); // Opens pmod connections
    set_conio_terminal_mode(); // turns on keyboard reading
    long last = 0;
    printf("Starting ESP32 BLE Test\r\n");
    ble_init(bt_uart);

    uint8_t instruction[8] = { 0x12, 0x34, 0x56, 0x78, 0x54, 0x23, 0x08, 0x04 };

    long message_end = 0;
    long message_start = 0;
    long average_time = 0;
    int trial_count = 0;
    int trail_amount = 0;

    while(1){
        ble_uart_check(bt_uart);
        char rx_buffer[256]; 

        if (uart_queue_pop(&uart_queue, rx_buffer) == 0){
            printf("[UART OUTPUT] %s\r\n", rx_buffer);
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

                if (send_at_cmd(bt_uart, "AT+BLECONNPARAM?\r\n", "+BLECONNPARAM:", conn_results, 1000) == 0) {
                    //printf("Current Params: %s\r\n", conn_results);
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
            //ble_send_instruction(bt_uart, instruction);
            ble_send_pkt(bt_uart, payload, 156);
            last = now;
        }
        
        
    }

    close(bt_uart);
    return 0;
}
