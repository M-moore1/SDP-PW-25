#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>

#include "includes/cmd_structure.h"
#include "../includes/ble/pmod_esp32.h"
#include "includes/ble/uart_queue.h"
#include "includes/cmd_parser/cmd_parser.h"
#include "includes/json_uds/json_uds.h"
#include "cJSON.h"

#define DEFAULT_UDS_PATH "/tmp/gs_bridge.sock"
#define DEFAULT_UART_DEV "/dev/ttyPS2"
#define MAX_TRACKED_CMDS 64

// =========================
// LATENCY TRACKING
// =========================
typedef struct {
    uint16_t id;
    uint64_t timestamp_ms;
    int valid;
} cmd_tracker_t;

static cmd_tracker_t cmd_buffer[MAX_TRACKED_CMDS];

extern uint64_t get_now_ms();

void store_command(uint16_t id) {
    uint64_t now = get_now_ms();

    for (int i = 0; i < MAX_TRACKED_CMDS; i++) {
        if (!cmd_buffer[i].valid) {
            cmd_buffer[i].id = id;
            cmd_buffer[i].timestamp_ms = now;
            cmd_buffer[i].valid = 1;
            return;
        }
    }

    cmd_buffer[0].id = id;
    cmd_buffer[0].timestamp_ms = now;
    cmd_buffer[0].valid = 1;
}

void process_received_id(uint16_t id, int uds_fd) {
    uint64_t now = get_now_ms();

    for (int i = 0; i < MAX_TRACKED_CMDS; i++) {
        if (cmd_buffer[i].valid && cmd_buffer[i].id == id) {

            uint64_t latency = now - cmd_buffer[i].timestamp_ms;
            cmd_buffer[i].valid = 0;

            cJSON *root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "type", "LATENCY");
            cJSON_AddNumberToObject(root, "id", id);
            cJSON_AddNumberToObject(root, "latency_ms", latency);

            char *json = cJSON_PrintUnformatted(root);
            uds_send_json(uds_fd, json);

            free(json);
            cJSON_Delete(root);
            return;
        }
    }
}

// =========================
// BLE NOTIFY PARSER
// =========================
void parse_notify_and_process(char *line, int uds_fd) {

    int conn, srv, chr, ascii_len;

    if (sscanf(line, "+NOTIFY:%d,%d,%d,%d,", &conn, &srv, &chr, &ascii_len) != 4)
        return;

    char *p = line;
    for (int i = 0; i < 4; i++) {
        p = strchr(p, ',');
        if (!p) return;
        p++;
    }

    int byte_len = ascii_len / 2;
    uint8_t data[512];

    for (int i = 0; i < byte_len; i++) {
        sscanf(&p[i * 2], "%2hhx", &data[i]);
    }


    uint8_t *payload = &data[1];

    robot_bt_packet_t pkt;
    memcpy(pkt.bytes, payload, 8);

    cJSON *json = robot_packet_to_json(pkt);

    if (json && uds_fd >= 0) {
        char *json_str = cJSON_PrintUnformatted(json);
        uds_send_json(uds_fd, json_str);
        free(json_str);
    }

    if (json) cJSON_Delete(json);

    uint16_t id = 0;

    switch (pkt.ctrl.type) {
        case System_CMD: id = pkt.sys.id; break;
        case Query_CMD: id = pkt.query.id; break;
        case ACK_CMD: id = pkt.ack.id; break;
    }

    if (id != 0)
        process_received_id(id, uds_fd);
}

// =========================
// JSON CHECK
// =========================
int looks_like_json(const char *s) {
    if (!s) return 0;
    while (*s == ' ' || *s == '\n' || *s == '\r' || *s == '\t') s++;
    return (*s == '{' || *s == '[');
}

// =========================
// MAIN
// =========================
int main(int argc, char **argv) {

    setvbuf(stdout, NULL, _IOLBF, 0);

    const char *uart_dev = DEFAULT_UART_DEV;
    if (argc >= 2) uart_dev = argv[1];

    int uart_fd = uart_open_config(uart_dev, DEFAULT_UART_BAUD);
    if (uart_fd < 0) {
        perror("UART open failed");
        return 1;
    }

    ble_init(uart_fd);

    int uds_listen = uds_server_listen(DEFAULT_UDS_PATH);
    if (uds_listen < 0) return 1;

    int uds_client = -1;
    int bt_connected = 0;

    while (1) {

        // =========================
        // ACCEPT UI
        // =========================
        if (uds_client < 0) {
            printf("Waiting for Node...\n");
            uds_client = accept(uds_listen, NULL, NULL);
            if (uds_client < 0) continue;

            fcntl(uds_client, F_SETFL, O_NONBLOCK);

            printf("Node connected\n");

            if (!bt_connected) {
                bt_connected = 1;
                ble_connect(uart_fd, ESP32_MAC);
            }
        }

        // =========================
        // SELECT SETUP
        // =========================
        fd_set rfds;
        FD_ZERO(&rfds);

        if (uds_client >= 0)
            FD_SET(uds_client, &rfds);

        FD_SET(uart_fd, &rfds);

        int maxfd = uart_fd;
        if (uds_client > maxfd) maxfd = uds_client;

        struct timeval tv = {0, 20000};

        int rc = select(maxfd + 1, &rfds, NULL, NULL, &tv);
        if (rc < 0) {
            if (errno == EINTR) continue;
            perror("select");
            break;
        }

        // =========================
        // UI → ROBOT
        // =========================
        if (uds_client >= 0 && FD_ISSET(uds_client, &rfds)) {

            uint32_t len_be;
            int r = read_full(uds_client, &len_be, 4);

            if (r == 0) {
                close(uds_client);
                uds_client = -1;
                continue;
            }

            if (r > 0) {

                uint32_t len = ntohl(len_be);
                if (len == 0 || len > 4096) continue;

                char *buf = malloc(len + 1);
                read_full(uds_client, buf, len);
                buf[len] = '\0';

                if (looks_like_json(buf)) {
                    cJSON *probe = cJSON_Parse(buf);
                    if (probe) {
                        cJSON_Delete(probe);

                        handle_node_json(uart_fd, uds_client, buf);

                        // OPTIONAL: extract ID here and track
                        // store_command(id);
                    }
                } else {
                    handle_encrypted_data(uart_fd, uds_client, buf);
                }

                free(buf);
            }
        }

        // =========================
        // ROBOT → UI (FIXED)
        // =========================
        if (FD_ISSET(uart_fd, &rfds)) {

            ble_uart_check(uart_fd);

            char tmp[256];

            if (uart_queue_pop(&uart_queue, tmp) == 0) {

                static char line[4096];
                static int idx = 0;

                int len = strlen(tmp);

                for (int i = 0; i < len; i++) {

                    char c = tmp[i];

                    if (c == '\n') {
                        line[idx] = '\0';

                        printf("[UART] %s\n", line);

                        if (strstr(line, "+NOTIFY:") && uds_client >= 0) {
                            parse_notify_and_process(line, uds_client);
                        }

                        idx = 0;
                    }
                    else {
                        if ((size_t)idx < sizeof(line) - 1)
                            line[idx++] = c;
                        else
                            idx = 0;
                    }
                }
            }
        }
    }

    return 0;
}