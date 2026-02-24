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
//gcc -O2 -Wall -Wextra test_2.c includes/bt2/bt2.c -I./includes/bt2 -I./includes/cmd_structure -o test_2



int main() {

    int bt_uart = uart_open_config(DEFAULT_UART_DEV, DEFAULT_UART_BAUD); // Opens pmod connections
    set_conio_terminal_mode(); // turns on keyboard reading
    
    long last = 0;

    int speed = 50;
    int results;
    int send_security_update = 0;

    robot_bt_packet_t ctrl_inst = {0};
    robot_bt_packet_t sys_inst = {0};
    sys_inst.sys.pl = 1;            sys_inst.sys.ac = 0x3FF;
    sys_inst.sys.type = System_CMD; 

    while(1){
        unsigned char rx_buffer[256]; 
        ssize_t bytes_read = read(bt_uart, rx_buffer, sizeof(rx_buffer));
        
        if (bytes_read > 0) {
            printf("[RX BYTES]: ");
            for (ssize_t i = 0; i < bytes_read; i++) {
                
                printf("%02X ", rx_buffer[i]);
            }
            printf("\n");
            fflush(stdout);
        }

        if(kbhit()){
            char c = getch();
            if (c == 'q')break;

            if (c == 'w') ctrl_inst.ctrl.w = 1;
            if (c == 'a') ctrl_inst.ctrl.a = 1;
            if (c == 's') ctrl_inst.ctrl.s = 1;
            if (c == 'd') ctrl_inst.ctrl.d = 1; 
            if (c == '+') { if (speed < 100) speed += 1; }
            if (c == '-'){ if (speed > 0) speed -= 1; }

            if(c == 'c'){
                printf("\nAttempting to connect...\r\n");
                results = rn42_connect_mac(bt_uart, "004B1224B0A6");
                if (results == 0){
                    printf("\nAttempt Successful\n");
                }else{
                    printf("\nAttempt Failed");
                }

            }
            if(c == 'v'){
                printf("\nDisconnecting\r\n");
                results = rn42_disconnect(bt_uart);
                if (results == 0){
                    printf("\nDisconnection Successful\n");
                }else{
                    printf("\nDisconnection Failed");
                }
            }
            if (c == '1'){
                send_security_update = 1;
                sys_inst.sys.instruction = SECURITY_LEVEL;
                sys_inst.sys.id = 0x05;
                sys_inst.sys.specific = 0x01;
                printf("\nSetting Secuirty to On\n");
            }
            if (c == '2'){
                send_security_update = 1;
                sys_inst.sys.instruction = SECURITY_LEVEL;
                sys_inst.sys.id = 0x05;
                sys_inst.sys.specific = 0x00;
                printf("\nSetting Secuirty to Off\n");
            }

        }
        
        long now = clock() / (CLOCKS_PER_SEC / 1000);
        if (now - last >= 500){
            //instruction set to type C 0b00001 in bits 0-4
            // print the the 64 bit instruction
            ctrl_inst.ctrl.pl = 1; ctrl_inst.ctrl.type = CONTROL_CMD; ctrl_inst.ctrl.speed = speed;
            uart_send_instruction(bt_uart, ctrl_inst.raw);
            ctrl_inst.raw = 0;

            if(send_security_update){
                uart_send_instruction(bt_uart, sys_inst.raw);
            }

            last  = now;
        }
        
    }

    close(bt_uart);
    return 0;
}
