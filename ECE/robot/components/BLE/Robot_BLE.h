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
#include "pinout.h"
#include "robot_commands.h"
//#include "aes_gcm_encrypt.h"

#define ROBOT_PROFILE_NUM                       1
#define ROBOT_PROFILE_APP_IDX                   0
#define ESP_ROBOT_APP_ID                        0x55
#define DEVICE_NAME                             "ROBOT_ESP32"
#define SVC_INST_ID                             0
#define CHAR_DECLARATION_SIZE                   (sizeof(uint8_t))
#define GATTS_DEMO_CHAR_VAL_LEN_MAX             500

#define MAX_DEVICES     2
#define CONN_ID_INVALID 0xFFFF

typedef struct {
    uint16_t conn_id;
    esp_gatt_if_t gatts_if;
    bool notify_enabled;
    uint8_t rx_buf[200];
    int rx_idx;
    uint8_t data_mode;
} device_conn_t;

extern device_conn_t connected_devices[MAX_DEVICES];
extern int num_connected;
extern QueueHandle_t ble_recieve_queue;

enum
{
    ROBOT_IDX_SVC,
    ROBOT_IDX_CHAR,
    ROBOT_IDX_VAL,
    ROBOT_IDX_CFG,
    ROBOT_IDX_NB,
};

extern uint16_t robot_handle_table[ROBOT_IDX_NB];
extern esp_ble_adv_data_t adv_data;
extern esp_ble_adv_params_t adv_params;
extern uint32_t spp_handle;

void robot_ble_init();
void send_bytes(uint8_t *packet, size_t len);
void send_bytes_to_all(uint8_t *packet, size_t len);
void send_string(char *txt);
void send_cmd(uint8_t* pkt, int sec_lvl);
void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

#endif