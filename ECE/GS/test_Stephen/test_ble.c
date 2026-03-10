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
//gcc -O2 -Wall -Wextra test_ble.c ../includes/ble/ble.c -I../includes -o test_ble


#define byte_test_size 156   
#define AVG_SAMPLES    50    
#define SEND_INTERVAL  50

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
    
    while(1){
        unsigned char rx_buffer[256]; 
        unsigned char output[256]; 

       ssize_t bytes_read = read(bt_uart, rx_buffer, sizeof(rx_buffer) - 1); // leave space for '\0'
        if (bytes_read > 0) {
            for (bytes_read){
                if(bytes_read[i] = '%'){
                    if(start == 0){
                        output[0] = bytes_read[i];
                        output_indx = 1;
                        start = 1;
                    }else{
                        output[output_indx] = '%';
                        output[output_indx+1] = '\0';
                        print(output)
                        output_indx = 0;
                        start = 0;
                    }
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

        }

        
        
    }

    close(bt_uart);
    return 0;
}
