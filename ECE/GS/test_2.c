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
    int connected = 0;
    robot_bt_packet_t ack_inst;
    int send = 0;

    while(1){
        unsigned char rx_buffer[256]; 
       

        ssize_t bytes_read = read(bt_uart, rx_buffer, sizeof(rx_buffer));
        
        if (bytes_read > 0) {
            
            memcpy(&ack_inst.raw, rx_buffer, sizeof(uint64_t));
            if(ack_inst.ack.type == ACK_CMD){
                printf("Robot Responsexcvcv\r\n");
                
            }
            printf("[RX BYTES]: ");
            for (ssize_t i = 0; i < bytes_read; i++) {
                
                printf("%02X", rx_buffer[i]);
            }
            printf("\r\n");
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
                printf("Entering CMD MODE\r\n");
                results = rn42_enter_cmd(bt_uart);
                if (results == 0){
                    printf("CMD Mode Entered Successfully\r\n");
                }else if (results == 1){
                    printf("Already in CMD Mode\r\n");
                }else{
                    printf("Failed to Enter CMD Mode\r\n");
                }
                
            }
            if(c == 'v'){
                results = rn42_exit_cmd(bt_uart);
                if (results == 0){
                    printf("Exited CMD Mode Successfully\r\n");
                }else{
                    printf("Failed to Exit CMD Mode\r\n");
                }
            }
            // ESP32 #1 004B1224B0A6
            // ESP32 ROBOT: 
            if (c == 'z'){
                printf("\nAttempting to connect...\r\n");
                results = rn42_connect_mac(bt_uart, "441d64f17066");
                if (results == 0){
                    printf("\nAttempt Successful\r\n");
                    connected = 1;
                }else{
                    printf("\nAttempt Failed\r\n");
                }
            }
            if(c == 'x'){
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


            if (c == '1'){
                send_security_update = 1;
                sys_inst.sys.instruction = SECURITY_LEVEL;
                sys_inst.sys.id = 0x05;
                sys_inst.sys.specific = 0x01;
                printf("Setting Secuirty to On\r\n");
            }
            if (c == '2'){
                send_security_update = 1;
                sys_inst.sys.instruction = SECURITY_LEVEL;
                sys_inst.sys.id = 0x05;
                sys_inst.sys.specific = 0x00;
                printf("Setting Secuirty to Off\r\n");
            }
            send = 1;
        }
        
        long now = clock() / (CLOCKS_PER_SEC / 1000);
        if (now - last >= 60){
            if (connected == 1){
                if(send == 1){
                    //instruction set to type C 0b00001 in bits 0-4
                    // print the the 64 bit instruction
                    ctrl_inst.ctrl.pl = 1; ctrl_inst.ctrl.type = CONTROL_CMD; ctrl_inst.ctrl.speed = speed;
                    uart_send_instruction(bt_uart, ctrl_inst.raw);
                    
                    ctrl_inst.raw = 0;

                    if(send_security_update){
                        uart_send_instruction(bt_uart, sys_inst.raw);
                    }
                    send = 0;
                }
                
            }
            last  = now;
        }
        
        
    }

    close(bt_uart);
    return 0;
}
