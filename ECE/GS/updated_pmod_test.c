#include <fcntl.h>
#include "../conio.c" 
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>

#include "../includes/ble/uart_queue.h"
#include "../includes/ble/pmod_esp32.h"
#include "../includes/cmd_structure.h"
#include "../cJSON-master/cJSON.h"

#define byte_test_size  156
#define AVG_SAMPLES    100   
#define SEND_INTERVAL  1000

// ===================== NEW PARSER =====================
void parse_notify_and_print(char *line) {

    int conn, srv, chr, ascii_len;

    if (sscanf(line, "+NOTIFY:%d,%d,%d,%d,", &conn, &srv, &chr, &ascii_len) != 4) {
        printf("Parse header failed\n");
        return;
    }

    char *p = line;
    for (int i = 0; i < 4; i++) {
        p = strchr(p, ',');
        if (!p) return;
        p++;
    }

    int byte_len = ascii_len / 2;
    uint8_t data[512];

    // HEX → BYTES
    for (int i = 0; i < byte_len; i++) {
        sscanf(&p[i * 2], "%2hhx", &data[i]);
    }

    printf("\n========== RAW BYTES ==========\n");
    printf("Len: %d\n", byte_len);

    for (int i = 0; i < byte_len; i++) {
        printf("%02X ", data[i]);
        if ((i+1)%16==0) printf("\n");
    }
    printf("\n");

    // Frame check
    if (data[0] != 0x0A || data[byte_len-1] != 0x0D) {
        printf("Invalid frame\n");
        return;
    }

    uint8_t *payload = &data[1];

    robot_bt_packet_t pkt;
    memcpy(pkt.bytes, payload, 8);

    printf("\n========== DECODE ==========\n");
    printf("RAW: 0x%016lX\n", pkt.raw);
    printf("TYPE: %d\n", pkt.ctrl.type);
    printf("PL: %d\n", pkt.ctrl.pl);

    uint16_t id = 0;

    switch(pkt.ctrl.type) {

        case System_CMD:
            id = pkt.sys.id;
            printf("System ID: %d\n", id);
            break;

        case Query_CMD:
            id = pkt.query.id;
            printf("Query ID: %d\n", id);
            break;

        case ACK_CMD:
            id = pkt.ack.id;
            printf("ACK ID: %d\n", id);
            break;

        case ROBOT_UPDATE_CMD:
            printf("Robot Update Part: %d\n", pkt.nav.part);
            break;

        default:
            printf("Unknown type\n");
            break;
    }

    // JSON BUILD
    cJSON *root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "type", "PARSED");
    cJSON_AddNumberToObject(root, "cmd_type", pkt.ctrl.type);
    cJSON_AddNumberToObject(root, "pl", pkt.ctrl.pl);
    cJSON_AddNumberToObject(root, "id", id);

    char *json = cJSON_PrintUnformatted(root);

    printf("\n========== JSON ==========\n");
    printf("%s\n", json);

    free(json);
    cJSON_Delete(root);
}
// =====================================================

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

    int bt_uart = uart_open_config(DEFAULT_UART_DEV, DEFAULT_UART_BAUD);
    set_conio_terminal_mode();

    long last = 0;
    printf("Starting ESP32 BLE Test\r\n");
    ble_init(bt_uart);

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
                // 🔥 NEW FUNCTION CALL
                parse_notify_and_print(rx_buffer);

                message_end = (clock() / (CLOCKS_PER_SEC / 1000000));
                long latency = message_end - message_start;

                average_time += latency;
                trial_count++;

                if (trial_count >= AVG_SAMPLES) {
                    long avg = average_time / trial_count;
                    printf("\r\n=== STATS #%d ===\r\n", trail_amount);
                    printf("Average Latency: %.3f ms\r\n", avg / 1000.0);
                    printf("==========================\r\n\n");

                    average_time = 0;
                    trial_count = 0;
                    trail_amount++;
                }
            }
        }

        if(kbhit()){
            char c = getch();

            if (c == 'q') break;

            if (c == 'z') ble_connect(bt_uart, NULL);
            if (c == 'x') ble_discon(bt_uart);
        }

        long now = clock() / (CLOCKS_PER_SEC / 1000000);

        if (now - last >= SEND_INTERVAL * 1000) {
            message_start = now;
            ble_send_pkt(bt_uart, payload, 156);
            last = now;
        }
    }

    close(bt_uart);
    return 0;
}