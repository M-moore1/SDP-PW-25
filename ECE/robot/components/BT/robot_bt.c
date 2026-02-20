#include "robot_bt.h"

static uint32_t spp_handle = 0;
char rx_buf[128];
int rx_index = 0;

QueueHandle_t cmd_queue = NULL;

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
            printf("DATA WAS RECEIVED ENTERING CASE\n");
            for (int i = 0; i < param->data_ind.len; i++) {

                char c = param->data_ind.data[i];
                printf("SANITY CHECK: Looping \n");
                if (c == '\r' || c == '\n') {
                    if (rx_index > 0) {
                        printf("BUFFER COMPLETE PRINTING FINAL MESSAGE\n");
                        rx_buf[rx_index] = '\0';
                        printf("%s", rx_buf);
                        // Decrypt
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
                    printf("MADE IT TO ADD TO BUFFER\n");
                    rx_buf[rx_index++] = c;
                }
                else {
                    printf("Buffer Overflow\n");
                    ESP_LOGW(TAG, "RX buffer overflow");
                    rx_index = 0;
                }
            }
            break;

        default:
            break;
    }
}