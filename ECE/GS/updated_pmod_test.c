
// gcc -O2 -Wall -Wextra updated_pmod_test.c includes/ble/pmod_esp32.c includes/ble/uart_queue.c cJSON-master/cJSON.c -I./includes -I./includes/ble -I./includes/cmd_structure -I./cJSON-master -o updated_pmod_test
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

#define AVG_SAMPLES 100
#define SEND_INTERVAL 1000
//gcc -O2 -Wall -Wextra updated_pmod_test.c \
includes/ble/pmod_esp32.c \
includes/ble/uart_queue.c \
cJSON-master/cJSON.c \
-I./includes \
-I./includes/ble \
-I./includes/cmd_structure \
-I./cJSON-master \
-o updated_pmod_test
// ===================== FIXED STREAM PARSER =====================
void parse_notify_and_print(char *line) {


int conn, srv, chr, byte_len;

if (sscanf(line, "+NOTIFY:%d,%d,%d,%d,", &conn, &srv, &chr, &byte_len) != 4) {
    printf("Parse header failed\n");
    return;
}

// Move pointer to raw binary data
char *p = line;
for (int i = 0; i < 4; i++) {
    p = strchr(p, ',');
    if (!p) return;
    p++;
}

uint8_t *data = (uint8_t *)p;

printf("\n========== RAW BYTES ==========\n");
printf("Len: %d\n", byte_len);

for (int i = 0; i < byte_len; i++) {
    printf("%02X ", data[i]);
    if ((i+1)%16==0) printf("\n");
}
printf("\n");

// ===============================
//  DIRECT PARSE (NO FRAMING)
// ===============================
if (byte_len < 8) {
    printf("Packet too small\n");
    return;
}

uint8_t *payload = data;

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

// ===============================
// JSON OUTPUT
// ===============================
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

uint8_t instruction[8] = { 0x12, 0x34, 0x56, 0x78, 0x54, 0x23, 0x08, 0x04 };

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

    char rx_buffer[512]; 

    if (uart_queue_pop(&uart_queue, rx_buffer) == 0){

        printf("[UART OUTPUT] %s\r\n", rx_buffer);

        if (strstr(rx_buffer, "NOTIFY") != NULL) 
        {
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
        ble_send_instruction(bt_uart, instruction);
        last = now;
    }
}

close(bt_uart);
return 0;

}
