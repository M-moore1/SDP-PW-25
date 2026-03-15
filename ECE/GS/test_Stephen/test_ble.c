#include <fcntl.h>
#include "../conio.c" 
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <stdint.h>
#include <time.h>
#include "../includes/ble/ble.h"
#include "../includes/cmd_structure.h"
#include "../includes/ble/uart_queue.h"
//gcc -O2 -Wall -Wextra test_ble.c ../includes/ble/ble.c ../includes/ble/uart_queue.c -I../includes -o test_ble.o


#define byte_test_size  156
#define AVG_SAMPLES    50    
#define SEND_INTERVAL  50

int main() {

    int bt_uart = uart_open_config(DEFAULT_UART_DEV, B921600); 
    int test = ble_init(bt_uart);
    set_conio_terminal_mode();
    
    long last = 0;

    int speed = 50;
    int results;
    int send_security_update = 0;

    int connected = 0;
    robot_bt_packet_t ack_inst;
    int send = 0;
    long message_end = 0;
    long message_start = 0;
    long average_time = 0;
    long average_idx = 0;
    int trial_count = 0;
    int trail_amount = 0;

    int start = 0;
    int output_indx = 0;
    unsigned char output[256];

    uint8_t packet[byte_test_size];
    for (int i = 0; i < byte_test_size; i++) {
        packet[i] = (uint8_t)(i % 255); 
    }

    char hex_string[(byte_test_size * 2) + 1];
    for (int i = 0; i < byte_test_size; i++) {
        sprintf(&hex_string[i * 2], "%02X", packet[i]);
    }
    
    char *msg_send = "8BDF573D3D50AF81FAD29D955B91D6BC09FF99709565F114ACDA91469379B86EC63824B37339EEC7AA903929137BD367D00B0A99534505FB3E603E24D02D77B1DBECA6931B981D18AE316FA14F2B32B8CB778305174D6961FDD3BEA0BE071223C75C3856286788AAEA14BCCD93A3F76FD1DDDD58C824E3467B73FF81F4D4AFAD61DB4398F119542F136E2C6AAB3B68615BCE9D26E148F04226C4994C";
    while(1){
        ble_uart_check(bt_uart);
        char msg[256];
        if (uart_queue_pop(&uart_queue, msg) == 0)
        {
            if (strstr(msg, "NOTI") != NULL) 
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

            // ESP32 #1 004B1224B0A6
            // ESP32 ROBOT: 44:1D:64:F1:1A:86
            if(c == 'c'){
                printf("Entering CMD MODE\r\n");
                results = ble_enter_cmd(bt_uart);
                if (results == 0){
                    printf("CMD Mode Entered Successfully\r\n");
                }else if (results == 1){
                    printf("Already in CMD Mode\r\n");
                }else{
                    printf("Failed to Enter CMD Mode\r\n");
                }
                
            }

            if(c == 'v'){
                results = ble_exit_cmd(bt_uart);
                if (results == 0){
                    printf("Exited CMD Mode Successfully\r\n");
                }else{
                    printf("Failed to Exit CMD Mode\r\n");
                }
            }

            if (c == 'z'){
                printf("\nAttempting to connect...\r\n");
                // NEW MAC
                results = ble_connect_mac(bt_uart, "441D64F11A86");
                if (results == 0){
                    printf("\nAttempt Successful\r\n");
                    connected = 1;
                }else{
                    printf("\nAttempt Failed\r\n");
                    connected = 0;
                }
            }
            if(c == 'x'){
                printf("Disconnecting\r\n");
                results = ble_disconnect(bt_uart);
                if (results == 0){
                    printf("Disconnection Successful\r\n");
                    connected = 0;
                }else if (results == 1){
                    printf("Not Connected\r\n");
                }else{
                    printf("Disconnection Failed\r\n");
                }
            }
            if (c == 'b'){
                printf("Checking Connection Status\r\n");
                results = ble_connect_check(bt_uart);
                if(results == 0){
                    printf("Connected\r\n");
                }else{
                    printf("Not Connected\r\n");
                }
            }
            if( c == 'a'){
                
                printf("MSG: %s \r\n", hex_string);
                message_start = clock() / (CLOCKS_PER_SEC / 1000000);
                uart_send_str(bt_uart, hex_string, strlen(hex_string));
                
            }

        }

        long now = clock() / (CLOCKS_PER_SEC / 1000); 
        
        if (now - last >= SEND_INTERVAL) { 
            
            if (connected){
                message_start = clock() / (CLOCKS_PER_SEC / 1000000);
                uart_send_str(bt_uart, msg_send, strlen(msg_send));
            }          
            
            
            last = now;
        } 
        
        
    }

    close(bt_uart);
    return 0;
}
