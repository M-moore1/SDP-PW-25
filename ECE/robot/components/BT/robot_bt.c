#include "robot_bt.h"

uint32_t spp_handle = 0;
uint8_t rx_buf[128]; // Change
int rx_idx = 0;

QueueHandle_t bt_recieve_queue = NULL;

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


void bt_init(){
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

    esp_bt_gap_set_device_name("SDP2635_ESP32");
    esp_bt_gap_set_scan_mode(
        ESP_BT_CONNECTABLE,
        ESP_BT_GENERAL_DISCOVERABLE
    );

    print_bt_mac();

    bt_recieve_queue= xQueueCreate(10, 128); // CHANGE

    ESP_LOGI(TAG, "SPP Server Ready");
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
            // CHANGE ENTIRE LOOP
            for (int i = 0; i < param->data_ind.len; i++) {
                uint8_t byte = param->data_ind.data[i];
                printf("rx_buf[%d] = 0x%02X\n", rx_idx, byte);

                rx_buf[rx_idx] = byte;
                rx_idx++;

                if (rx_idx == 156) {
                    if(rx_buf[155] == 0x0D) {
                        printf("rx_buf: ");
                        for (int j = 0; j < 128; j++) {
                            printf("%02X ", rx_buf[j]);
                        }
                        printf("\n");


                        
                        uint8_t processed_cmd[128];
                        memcpy(processed_cmd, rx_buf, 128);
                        
                        // Using PRIx64 to print the full 64-bit result
                        //printf(">>> Full Command: 0x%016llX\n", (unsigned long long)full_command);

                        xQueueSend(bt_recieve_queue, &full_command, 0);
                    }
                    
                    // Reset for next packet
                    rx_idx = 0; 
                }
            }
            break;
            
        default:
            break;
    }
}