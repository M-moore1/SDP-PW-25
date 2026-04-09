#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "driver/uart.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "pinout.h"
#include "Robot_BLE.h"
#include "stepper_motor.h"
#include "robot_commands.h"
#include "i2c.h"
#include "imu.h"
//#include "aes_gcm_decrypt.h"

#include "esp_spiffs.h"
#include "nvs_flash.h"


step_mot_t front_left;
step_mot_t front_right;
step_mot_t back_left;
step_mot_t back_right;

static QueueHandle_t cmd_queue = NULL;

void ble_recieve_parser(void *pvParameters)
{
    uint8_t received_packet[PACKET_SIZE]; 
    
    while (1) {
        robot_bt_packet_t cmd_packet = {0};
        int tag_check = -1;

        if (xQueueReceive(ble_recieve_queue, &received_packet, portMAX_DELAY))
        {   
            ESP_LOGI(MAIN_TAG, "RECIEVED BLE DATA");
            
            if (security_flag) {
                ESP_LOGI(MAIN_TAG, "Secure Mode - Decrypting Data");
                /*
                char plaintext[256];
                size_t pt_len = 0;
                tag_check = aes_gcm_decrypt_packet(received_packet, plaintext, &pt_len);

                if (tag_check == 0) {
                    memcpy(cmd_packet.bytes, plaintext, 8);
                    ESP_LOGI(MAIN_TAG, "Decrypted Hex: %02X %02X %02X %02X %02X %02X %02X %02X",
                         cmd_packet.bytes[0], cmd_packet.bytes[1], cmd_packet.bytes[2], cmd_packet.bytes[3],
                         cmd_packet.bytes[4], cmd_packet.bytes[5], cmd_packet.bytes[6], cmd_packet.bytes[7]);

                    memcpy(cmd_packet.bytes, plaintext, 8);
                } else {
                    ESP_LOGW(MAIN_TAG, "Secure Mode - Decryption Failed");
                    send_ack(0, RESULT_AUTH_FAIL, security_flag, NO_INFO);
                }
                */
            }else{
                memcpy(cmd_packet.bytes, received_packet, 8);
                tag_check = 0;
            }

            if (tag_check == 0){
                if (xQueueSend(cmd_queue, &cmd_packet, 0) != pdPASS) {
                    ESP_LOGW(MAIN_TAG, "cmd_queue full");
                }
            }
        }
    }
}

void command_parser(void *pvParameters)
{
    robot_bt_packet_t cmd;
    while (1)
    {
        if (xQueueReceive(cmd_queue, &cmd, portMAX_DELAY))
        {
            ESP_LOGI(MAIN_TAG, "Packet Parsing Initiated");

            command_type_t cmd_type = (command_type_t)cmd.ctrl.type;

            switch (cmd_type){
                case CONTROL_CMD:
                    ESP_LOGI(MAIN_TAG, "Control CMD Recieved");
                    control_cmd(cmd.ctrl, &front_left, &front_right, &back_left, &back_right);
                break;

                case ARM_CMD:
                    ESP_LOGI(MAIN_TAG, "Arm CMD Recieved");
                    arm_cmd(cmd.arm, &front_left, &front_right, &back_left, &back_right); // CHANGE MOTORS
                break;

                case System_CMD:
                    ESP_LOGI(MAIN_TAG, "System CMD Recieved");
                    system_cmd(cmd.sys, &front_left, &front_right, &back_left, &back_right);
                break;
                
                case Query_CMD:
                    ESP_LOGI(MAIN_TAG, "Query CMD Recieved");
                    query_cmd(cmd.query, &front_left, &front_right, &back_left, &back_right);
                break;
                
                default:
                    ESP_LOGW(MAIN_TAG, "Unknown CMD Recieved");
                    send_ack(0, RESULT_UNKNOWN_CMD, security_flag, NO_INFO);
                break;
            }
        }
    }
}



void app_main()
{
    /*
    i2c_master_bus_handle_t i2c_bus = i2c_init();
    i2c_master_dev_handle_t imu     = device_init(i2c_bus, IMU_ADDR);

    if (!bno08x_init(imu)) {
        ESP_LOGE(IMU_TAG, "Failed to init BNO08x, halting");
        while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }
    */

    /* ADDED SECURITY BLOCK */

    /* NEW: Initialize NVS and erase if corrupted or stale BLE security state */
    esp_err_t nvs_ret = nvs_flash_init();
    if (nvs_ret == ESP_ERR_NVS_NO_FREE_PAGES || 
        nvs_ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW("MAIN", "Erasing NVS flash to clear stale BLE security state...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs_ret);
    ESP_LOGI("MAIN", "NVS initialized successfully");
    /* END NEW */

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = false
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE("MAIN", "SPIFFS mount failed: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI("MAIN", "SPIFFS mounted successfully");
    }
    /* ------------------------------------------------------------ */

    robot_ble_init();                  // Initialize BLE

    motor_init(&front_left,  FL_MOTOR_STEP, FL_MOTOR_DIR, FL_MOTOR_EN, FL_MOTOR_PWM);
    motor_init(&front_right, FR_MOTOR_STEP, FR_MOTOR_DIR, FR_MOTOR_EN, FR_MOTOR_PWM);
    motor_init(&back_left,   BL_MOTOR_STEP, BL_MOTOR_DIR, BL_MOTOR_EN, BL_MOTOR_PWM);
    motor_init(&back_right,  BR_MOTOR_STEP, BR_MOTOR_DIR, BR_MOTOR_EN, BR_MOTOR_PWM);

    cmd_queue = xQueueCreate(10, sizeof(uint64_t));
    
    int64_t last_send_time = 0;
    int rotation_step = 0;
    int64_t now = esp_timer_get_time();

    xTaskCreatePinnedToCore( ble_recieve_parser, "ble_recieve_parser", 4096, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore( command_parser, "robot_command_parser", 4096, NULL, 5, NULL, 1);

    while (1) {
        /*
        if (gpio_get_level(IMU_INT_PIN) == 0) {
            while (gpio_get_level(IMU_INT_PIN) == 0) {
                imu_check_safe(imu);
            }
        }
        */
        
        if (device_connected && notify_enabled) {
            if (now - last_send_time >= 500000) {
        
                switch (rotation_step) {
                    case 0: send_imu(1); break;
                    case 1: send_imu(2); break;
                    case 2: send_imu(3); break;
                    case 3: send_health_report(); break;
                }

                rotation_step = (rotation_step + 1) % 4; 
                last_send_time = now;
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(500));
        
    }
    return;
}