#ifndef ROBOT_BT_H
#define ROBOT_BT_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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
#include "pinout.h"
#define TAG "SDP2635_SPP_Server"

extern uint32_t spp_handle;
extern uint8_t rx_buf[156];
extern int rx_index;
extern QueueHandle_t bt_recieve_queue;

void disable_bluetooth_auth(void);
void bt_init();
void print_bt_mac(void);
void bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);
void bt_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param);



#endif
