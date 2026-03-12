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


#define byte_test_size 156   
#define AVG_SAMPLES    50    
#define SEND_INTERVAL  60

int main() {

    int bt_uart = uart_open_config(DEFAULT_UART_DEV, DEFAULT_UART_BAUD); 
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

    int start = 0;
    int output_indx = 0;
    unsigned char output[256];

    char *msg_send = "464F5257415244";
    
    while(1){
        ble_uart_check(bt_uart);
        char msg[256];
        if (uart_queue_pop(&uart_queue, msg) == 0)
        {
            if (strstr(msg, "NOTI") != NULL) 
            {
                // 1. Capture end time and calculate current latency
                message_end = clock() / (CLOCKS_PER_SEC / 1000000);
                long latency = message_end - message_start;
                
                // 2. Accumulate for average
                average_time += latency;
                trial_count++;
                
                // 3. Check if we have enough samples to calculate average
                if (trial_count >= AVG_SAMPLES) {
                    long avg = average_time / trial_count;
                    printf("\r\n=== STATS [%d Samples] ===\r\n", trial_count);
                    printf("Average Latency: %.3f ms\r\n", avg / 1000.0);
                    printf("==========================\r\n\n");
                    
                    // Reset variables for the next batch
                    average_time = 0;
                    trial_count = 0;
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
                
                //printf("MSG: %s \r\n", msg_send);
                message_start = clock() / (CLOCKS_PER_SEC / 1000000);
                uart_send_str(bt_uart, msg_send, strlen(msg_send));
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
