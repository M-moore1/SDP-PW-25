#include <fcntl.h>
#include "conio.c" 
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <stdint.h>
#include <time.h>

int serial_port_init(char* port, speed_t baud_rate){

    int bt2_uart = open(port, O_RDWR | O_NDELAY |O_NOCTTY);
    if (bt2_uart < 0) {
        perror("Failed to connect to PMOD");
        return -1;
    }

    struct termios port_settings;
    
    port_settings.c_cflag = baud_rate | CS8 | CREAD | CLOCAL;
    port_settings.c_iflag = IGNPAR;

    tcflush (bt2_uart, TCIFLUSH);
    tcsetattr(bt2_uart, TCSANOW, &port_settings);

    return bt2_uart;
}

int main() {

    int bt_uart = serial_port_init("/dev/ttyPS2", B115200); // Opens pmod connections
    set_conio_terminal_mode(); // turns on keyboard reading
    
    long last = 0;
    int interval = 2;

    char movement_json[255];
    int send_index = 0;
    
    int w = 0, a = 0, s = 0, d = 0, speed = 50;
    uint64_t instruction = 0;
    int alt_instruction = 0;
    while(1){
        char rx_buffer[256];
        int bytes_read = read(bt_uart, &rx_buffer, sizeof(rx_buffer) - 1);
        if (bytes_read > 0) {
            rx_buffer[bytes_read] = '\0';
            printf("[RX]: %s", rx_buffer);
            fflush(stdout);
        }

        if(kbhit()){
            char c = getch();
            if (c == 'q')break; 
            if (c == 'w') w = 1;
            if (c == 'a') a = 1;
            if (c == 's') s = 1;
            if (c == 'd') d = 1;   
            if (c == '+') { if (speed < 100) speed += 1; }
            if (c == '-'){ if (speed > 0) speed -= 1; }

            

            if(c == 'c'){
                write(bt_uart, "$$$", 3);
                usleep(500000);
                char* conn_cmd = "C\r"; 
                write(bt_uart, conn_cmd, strlen(conn_cmd));
    
                printf("\nAttempting to connect...\r\n");

            }
            if(c == 'v'){
                write(bt_uart, "\r$$$", 4);
                usleep(500000);
                char* conn_cmd = "\rK,\r"; 
                write(bt_uart, conn_cmd, strlen(conn_cmd));
                usleep(500000);
                write(bt_uart, "\r---\r",5);

                printf("\nDisconnecting\r\n");
            }
            if(c == 'z'){
               write(bt_uart, "\r---\r",5); 
            }

            // put speed in bit 9-15 of instructions
        }
        
        long now = clock() / (CLOCKS_PER_SEC / 1000);
        if (now - last >= 500){
            //instruction set to type C 0b00001 in bits 0-4
            // print the the 64 bit instruction

            instruction = 0; 
            if (!alt_instruction){
                instruction |= (0x01 & 0x1F);
                if (w) instruction |= (1ULL << 5);
                if (a) instruction |= (1ULL << 6);
                if (s) instruction |= (1ULL << 7);
                if (d) instruction |= (1ULL << 8);
                instruction |= ((uint64_t)(speed & 0x7F) << 9);
                
            }else if (alt_instruction == 1){

            }

            char bit_string[66]; 

            for (int i = 0; i < 64; i++) {
                uint64_t mask = 1ULL << (63 - i);
                bit_string[i] = (instruction & mask) ? '1' : '0';
            }
            bit_string[64] = '\r';
            bit_string[65] = '\0';
            write(bt_uart, bit_string, 65);

            w = a = s = d = 0;
            last = now;
            
            printf("\r Instruction: 0x%016llX | Speed: %d\n", (unsigned long long)instruction, speed);
            printf("\rSent Binary: %s\n", bit_string);
        }
        
    }

    close(bt_uart);
    return 0;
}
