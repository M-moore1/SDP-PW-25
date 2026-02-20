#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "stepper_motor.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "driver/uart.h"

#include "nvs_flash.h"
#include "esp_log.h"
#include "robot_bt.h"
#include "pinout.h"

#define TAG "SPP_SERVER"



step_mot_t test_motor;



void cmd_parser_task(void *pvParameters)
{
    uint64_t inst;

    while (1) {
        if (xQueueReceive(cmd_queue, &inst, portMAX_DELAY)) {

            // Skip 0 then keep wherever a 1 is

            uint8_t type = (inst >> 0) & 0x1F;

            ESP_LOGI(TAG, "Received instruction type value: %u", type);

            switch (type) {
                case 1:
                    ESP_LOGI(TAG, "Control instruction received");
                    uint8_t W = (inst >> 5) & 0x01;
                    uint8_t A = (inst >> 6) & 0x01;
                    uint8_t S = (inst >> 7) & 0x01;
                    uint8_t D = (inst >> 8) & 0x01;

                    // Extract speed (bits 9-15)
                    uint8_t speed = (inst >> 9) & 0x7F;

                    // Extract priority level (bits 16-17)
                    uint8_t PL = (inst >> 16) & 0x03;

                    // Print the parsed values
                    ESP_LOGI(TAG, "W=%u A=%u S=%u D=%u", W, A, S, D);
                    if(W == 1) motor_pulse(&test_motor, speed, 1);
                    ESP_LOGI(TAG, "Speed=%u", speed);
                    ESP_LOGI(TAG, "Priority Level=%u", PL);
                    break;
                case 2:
                    ESP_LOGI(TAG, "Pose instruction received");
                    break;

                case 3:
                    ESP_LOGI(TAG, "System instruction received");
                    break;
                
                case 4:
                    ESP_LOGI(TAG, "Query instruction received");
                    break;
                default:
                    ESP_LOGW(TAG, "Unknown command type: %u", type);
                    break;
            }
        }
    }
}

/* -------------------- Utilities -------------------- */

void print_bt_mac(void)
{
    const uint8_t *mac = esp_bt_dev_get_address();
    if (mac) {
        ESP_LOGI(TAG,
            "BT MAC: %02X:%02X:%02X:%02X:%02X:%02X",
            mac[0], mac[1], mac[2],
            mac[3], mac[4], mac[5]);
    }
}

/* -------------------- app_main -------------------- */

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());

    esp_bt_controller_mem_release(ESP_BT_MODE_BLE);

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT));

    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    esp_bt_gap_register_callback(bt_gap_cb);
    esp_spp_register_callback(bt_spp_cb);

    disable_bluetooth_auth();

    esp_spp_cfg_t spp_cfg = {
        .mode = ESP_SPP_MODE_CB,
        .enable_l2cap_ertm = false,
        .tx_buffer_size = 0
    };
    ESP_ERROR_CHECK(esp_spp_enhanced_init(&spp_cfg));

    esp_bt_gap_set_device_name("STEVE_ESP32");
    esp_bt_gap_set_scan_mode(
        ESP_BT_CONNECTABLE,
        ESP_BT_GENERAL_DISCOVERABLE
    );

    print_bt_mac();

    cmd_queue = xQueueCreate(10, sizeof(uint64_t));

    xTaskCreatePinnedToCore(
        cmd_parser_task,
        "cmd_parser",
        4096,
        NULL,
        5,
        NULL,
        1
    );

    ESP_LOGI(TAG, "SPP Server Ready");
    motor_init(&test_motor, TEST_MOTOR_STEP, TEST_MOTOR_DIR, TEST_MOTOR_EN, TEST_MOTOR_CHANNEL);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
