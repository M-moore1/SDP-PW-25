#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "arm.h"

#define TEST_UART_NUM  UART_NUM_0
#define TEST_UART_BAUD 115200
#define UART_BUF_SIZE  256
#define ESC            0x1B
#define KEY_SPEED      64   // mid-range — tune in arm.h via ARM_SPEED_MIN/MAX_STEP

static const char* TAG = "ARM_TEST";

static char parse_escape(void) {
    uint8_t buf[8] = {0};
    int len = 0;
    TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(50);
    while (xTaskGetTickCount() < deadline && len < (int)sizeof(buf) - 1) {
        uint8_t c;
        if (uart_read_bytes(TEST_UART_NUM, &c, 1, pdMS_TO_TICKS(10)) <= 0) break;
        buf[len++] = c;
        if (len == 2 && buf[0] == '[') {
            switch (buf[1]) {
                case 'A': return 'U';
                case 'B': return 'D';
                case 'C': return 'R';
                case 'D': return 'L';
            }
        }
        if (len == 5 && buf[0]=='[' && buf[1]=='1' && buf[2]==';' && buf[3]=='2') {
            switch (buf[4]) {
                case 'A': return 'Z';
                case 'B': return 'z';
            }
        }
    }
    return 0;
}

void app_main(void) {
    uart_config_t cfg = {
        .baud_rate  = TEST_UART_BAUD,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_driver_install(TEST_UART_NUM, UART_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(TEST_UART_NUM, &cfg);

    arm_init();
    ESP_LOGI(TAG, "Arm test ready");

    printf("\r\n=== ARM TEST ===\r\n");
    printf("  Arrows   XY  |  Shift+Up/Down  Z  |  r reset  s status\r\n\r\n");

    float step = ARM_SPEED_MIN_STEP +
                 (KEY_SPEED / 127.0f) * (ARM_SPEED_MAX_STEP - ARM_SPEED_MIN_STEP);

    while (1) {
        uint8_t ch;
        if (uart_read_bytes(TEST_UART_NUM, &ch, 1, pdMS_TO_TICKS(100)) <= 0) continue;

        float x, y, z;
        arm_get_position(&x, &y, &z);
        float nx = x, ny = y, nz = z;
        bool do_move = false;

        if (ch == ESC) {
            char key = parse_escape();
            switch (key) {
                case 'U': nx += step; do_move = true; printf("\r  X+ "); break;
                case 'D': nx -= step; do_move = true; printf("\r  X- "); break;
                case 'R': ny += step; do_move = true; printf("\r  Y+ "); break;
                case 'L': ny -= step; do_move = true; printf("\r  Y- "); break;
                case 'Z': nz += step; do_move = true; printf("\r  Z+ "); break;
                case 'z': nz -= step; do_move = true; printf("\r  Z- "); break;
                default: break;
            }
        } else {
            switch (ch) {
                case 'r':
                    arm_reset();
                    printf("\r  Reset\r\n");
                    break;
                case 's': {
                    float px, py, pz;
                    arm_get_position(&px, &py, &pz);
                    printf("\r  X=%.2f Y=%.2f Z=%.2f\r\n", px, py, pz);
                    break;
                }
                default: break;
            }
            continue;
        }

        if (do_move) {
            if (arm_move_to(nx, ny, nz) != 0) {
                printf("✗ out of range\r\n");
            } else {
                float px, py, pz;
                arm_get_position(&px, &py, &pz);
                printf("✓  X=%.2f Y=%.2f Z=%.2f\r\n", px, py, pz);
            }
        }
    }
}