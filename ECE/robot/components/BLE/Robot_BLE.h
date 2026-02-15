// Robot_BLE.h
#ifndef ROBOT_BLE_H
#define ROBOT_BLE_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gatt_common_api.h"

#define GATTS_TABLE_TAG "ROBOT_SDP2635"
#define ROBOT_PROFILE_NUM                       1
#define ROBOT_PROFILE_APP_IDX                   0
#define ESP_ROBOT_APP_ID                        0x55
#define SAMPLE_DEVICE_NAME                      "ROBOT_ESP32"
#define SVC_INST_ID                             0
#define CHAR_DECLARATION_SIZE                   (sizeof(uint8_t))
#define GATTS_DEMO_CHAR_VAL_LEN_MAX             500

enum
{
    ROBOT_IDX_SVC,     // Service container for entire robot communication

    ROBOT_IDX_CHAR,    // Characteristic metadata (UUID + properties: READ/WRITE/NOTIFY)

    ROBOT_IDX_VAL,     // Actual data pipe 

    ROBOT_IDX_CFG,     // Client config descriptor (enables/disables NOTIFY)

    ROBOT_IDX_NB,      // Total number of attributes in this service
};

// BLE globals
extern uint16_t robot_conn_id;
extern esp_gatt_if_t robot_gatts_if;
extern bool device_connected;
extern bool notify_enabled;
extern uint16_t robot_handle_table[ROBOT_IDX_NB];
extern esp_ble_adv_data_t adv_data;
extern esp_ble_adv_params_t adv_params;
extern QueueHandle_t cmd_queue;

// Function declarations
void robot_ble_init();
void send_string(char *txt);
void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

#endif






















