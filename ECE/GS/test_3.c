#include <fcntl.h>
#include "conio.c" 
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <stdint.h>
#include <time.h>
#include "includes/bt2/bt2.h"
#include "includes/cmd_structure.h"
//gcc -O2 -Wall -Wextra test_3.c includes/bt2/bt2.c -I./includes/bt2 -I./includes/cmd_structure -o test_3


#define byte_test_size 156   
#define AVG_SAMPLES    50    
#define SEND_INTERVAL  50

int main() {

    int bt_uart = uart_open_config(DEFAULT_UART_DEV, DEFAULT_UART_BAUD); 
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

    while(1){
        unsigned char rx_buffer[256]; 
       

        ssize_t bytes_read = read(bt_uart, rx_buffer, sizeof(rx_buffer));
        if (bytes_read > 0) {
            message_end = clock() / (CLOCKS_PER_SEC / 1000000);
            /*
            printf("[RX]: ");
            for (ssize_t i = 0; i < bytes_read; i++) {
                
                printf("%c", rx_buffer[i]);
            }
            printf("\r\n");
            printf("[RoundTrip]: %ld us (%.3f ms)\r\n", rtt_us, (double)rtt_us / 1000.0);
            */
            if (message_start > 0) {
                long rtt_us = message_end - message_start;
                average_time += rtt_us; 
                average_idx++;


                if (average_idx >= AVG_SAMPLES) {
                    long final_avg = average_time / AVG_SAMPLES;
                    printf(" Stats for every %d packets Trial Number: %d\r\n", AVG_SAMPLES, trial_count);
                    printf(" Packet Size: %d bytes\r\n", byte_test_size+2);
                    printf(" Avg RTT:    %ld us (%.3f ms)\r\n\r\n", final_avg, (double)final_avg / 1000.0);

                
                    average_time = 0;
                    average_idx = 0;
                    trial_count++;
                }
            }
            fflush(stdout);
        }
        
        
        if(kbhit()){
            char c = getch();
            if (c == 'q')break;

            // ESP32 #1 004B1224B0A6
            // ESP32 ROBOT: 44:1D:64:F1:1A:86
            if (c == 'c'){
                printf("\nAttempting to connect...\r\n");
                results = rn42_connect_mac(bt_uart, "441D64F11A86");
                if (results == 0){
                    printf("\nAttempt Successful\r\n");
                    connected = 1;
                }else{
                    printf("\nAttempt Failed\r\n");
                }
            }
            if(c == 'v'){
                printf("Disconnecting\r\n");
                results = rn42_disconnect(bt_uart);
                if (results == 0){
                    printf("Disconnection Successful\r\n");
                    connected = 0;
                }else{
                    printf("Disconnection Failed\r\n");
                }
            }
            if (c == 'b'){
                results = rn42_connect_check(bt_uart);
                if (results == 0){
                    printf("Device Connected\r\n");
                    connected = 1;
                }else if (results == 1){
                    printf("Device Not Connected\r\n");
                    connected = 0;
                }else{
                    printf("Other Error\r\n");
                }
            }

        }

        long now = clock() / (CLOCKS_PER_SEC / 1000); 
        if (now - last >= SEND_INTERVAL) { 
            if (connected == 1) {
                uint8_t packet[byte_test_size];
                for (int i = 0; i < byte_test_size; i++) {
                    packet[i] = (uint8_t)(i % 255); 
                }
                
                /*
                printf("Sending 256 bytes to UART:\r\n");
                for (int i = 0; i < 156; i++) {
                    printf("%02X ", packet_156[i]);
                    if ((i + 1) % 16 == 0) printf("\r\n"); 
                }
                printf("\r\n");
                */

                
                message_start = clock() / (CLOCKS_PER_SEC / 1000000);
                int n = uart_send_encrypted(bt_uart, packet);
                
                if (n < 0) {
                    perror("UART Write Failed");
                }
            }
            last = now;
        } 
    }

    close(bt_uart);
    return 0;
}
