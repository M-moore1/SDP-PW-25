#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "stepper_motor.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "driver/uart.h"

#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_spp_api.h"

#define TAG "SPP_SERVER"
#define TEST_MOTOR_STEP GPIO_NUM_32
#define TEST_MOTOR_DIR  GPIO_NUM_33
#define TEST_MOTOR_EN   GPIO_NUM_25
#define TEST_MOTOR_CHANNEL LEDC_CHANNEL_0

/* -------------------- Globals -------------------- */

static uint32_t spp_handle = 0;

/* RX buffer for SPP hex string */
static char rx_buf[128];
static int rx_index = 0;

/* Command queue */
static QueueHandle_t cmd_queue = NULL;

step_mot_t test_motor;

/* -------------------- Bluetooth Security -------------------- */

void disable_bluetooth_auth(void)
{
    // Set IO capabilities to "No Input / No Output"
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_NONE;
    esp_bt_gap_set_security_param(
        ESP_BT_SP_IOCAP_MODE, &iocap, sizeof(iocap));

    // Set fixed PIN = 0000 (legacy fallback)
    esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_FIXED;
    esp_bt_pin_code_t pin_code = {'0','0','0','0'};
    esp_bt_gap_set_pin(pin_type, 4, pin_code);

    ESP_LOGI(TAG, "Bluetooth auth minimized (SPP_SEC_NONE, PIN 0000)");
}


/* -------------------- GAP Callback -------------------- */

void bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    switch (event) {

        case ESP_BT_GAP_AUTH_CMPL_EVT:
            ESP_LOGI(TAG, "Authentication complete");
            break;

        case ESP_BT_GAP_PIN_REQ_EVT: {
            esp_bt_pin_code_t pin_code = {'0','0','0','0'};
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, pin_code);
            break;
        }

        case ESP_BT_GAP_CFM_REQ_EVT:
            esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
            break;

        default:
            break;
    }
}

/* -------------------- SPP Callback -------------------- */

void bt_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
    switch (event) {

        case ESP_SPP_INIT_EVT:
            ESP_LOGI(TAG, "SPP initialized");
            esp_spp_start_srv(
                ESP_SPP_SEC_NONE,
                ESP_SPP_ROLE_SLAVE,
                0,
                "SPP_SERVER"
            );
            break;

        case ESP_SPP_SRV_OPEN_EVT:
            spp_handle = param->srv_open.handle;
            ESP_LOGI(TAG, "Client connected (handle=%lu)", spp_handle);
            break;

        case ESP_SPP_CLOSE_EVT:
            spp_handle = 0;
            ESP_LOGI(TAG, "Client disconnected");
            break;

        case ESP_SPP_DATA_IND_EVT:
            for (int i = 0; i < param->data_ind.len; i++) {

                char c = param->data_ind.data[i];

                if (c == '\r' || c == '\n') {
                    if (rx_index > 0) {

                        rx_buf[rx_index] = '\0';

                        char *end;
                        uint64_t inst = strtoull(rx_buf, &end, 2);

                        if (end != rx_buf) {
                            // Success! The string of 1s and 0s is now a uint64_t bitfield
                            if (xQueueSend(cmd_queue, &inst, 0) != pdPASS) {
                                ESP_LOGW(TAG, "Command queue full");
                            }
                        } else {
                            ESP_LOGW(TAG, "Invalid binary string: %s", rx_buf);
                        }

                        rx_index = 0;
                    }
                }
                else if (rx_index < sizeof(rx_buf) - 1) {
                    rx_buf[rx_index++] = c;
                }
                else {
                    ESP_LOGW(TAG, "RX buffer overflow");
                    rx_index = 0;
                }
            }
            break;

        default:
            break;
    }
}

/* -------------------- Command Parser Task -------------------- */

void cmd_parser_task(void *pvParameters)
{
    uint64_t inst;

    while (1) {
        if (xQueueReceive(cmd_queue, &inst, portMAX_DELAY)) {

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
