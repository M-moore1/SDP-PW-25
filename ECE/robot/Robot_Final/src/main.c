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
#include "aes_gcm_decrypt.h"
#include "arm.h"


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
            //ESP_LOGI(MAIN_TAG, "RECIEVED BLE DATA");
            
            if (security_flag) {
                ESP_LOGI(MAIN_TAG, "Secure Mode - Decrypting Data");
                
                char plaintext[256];
                size_t pt_len = 0;
                tag_check = aes_gcm_decrypt_packet(received_packet, plaintext, &pt_len);

                if (tag_check == 0) {
                    /*
                    ESP_LOGI(MAIN_TAG, "Decrypted Hex: %02X %02X %02X %02X %02X %02X %02X %02X",
                         cmd_packet.bytes[0], cmd_packet.bytes[1], cmd_packet.bytes[2], cmd_packet.bytes[3],
                         cmd_packet.bytes[4], cmd_packet.bytes[5], cmd_packet.bytes[6], cmd_packet.bytes[7]);
                    */
                    memcpy(cmd_packet.bytes, plaintext, 8);
                } else {
                    ESP_LOGW(MAIN_TAG, "Secure Mode - Decryption Failed");
                    send_ack(0, RESULT_AUTH_FAIL, security_flag, NO_INFO);
                }
                
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
                    control_cmd(cmd.ctrl, &front_left, &front_right, &back_left, &back_right);
                break;

                case ARM_CMD:
                    ESP_LOGI(MAIN_TAG, "Arm CMD Recieved");
                    arm_cmd(cmd.arm, &front_left, &front_right, &back_left, &back_right);
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
                    send_ack(0, RESULT_UNKNOWN_CMD, security_flag, NO_INFO);
                break;
            }

            //vTaskDelay(pdMS_TO_TICKS(1)); 
            
            step_mot_t* motors[] = { &front_left, &front_right, &back_left, &back_right };
            bool all_idle = true;
            for (int i = 0; i < 4; i++) {
                if (motors[i]->status == MOTOR_RUNNING) {
                    all_idle = false;
                    break;
                }
            }

            if (all_idle) {
                ESP_LOGI(MAIN_TAG, "All motors idle — disabling");
                stepper_disable(&front_left);
                stepper_disable(&front_right);
                stepper_disable(&back_left);
                stepper_disable(&back_right);
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
    ESP_ERROR_CHECK(nvs_flash_init()); // Initialize NVS
    robot_ble_init();                  // Initialize BLE

    motor_init(&front_left, FL_MOTOR_STEP, FL_MOTOR_DIR, FL_MOTOR_EN, FL_MOTOR_PWM, FL_MOTOR_TIMER );
    motor_init(&back_left, BL_MOTOR_STEP, BL_MOTOR_DIR, BL_MOTOR_EN, BL_MOTOR_PWM, BL_MOTOR_TIMER);
    motor_init(&front_right, FR_MOTOR_STEP, FR_MOTOR_DIR, FR_MOTOR_EN, FR_MOTOR_PWM, FR_MOTOR_TIMER );
    motor_init(&back_right, BR_MOTOR_STEP, BR_MOTOR_DIR, BR_MOTOR_EN, BR_MOTOR_PWM, BR_MOTOR_TIMER );
    arm_init();

    cmd_queue = xQueueCreate(10, sizeof(uint64_t)); // Initialize the command queue
    
    int64_t last_send_time = 0;
    int rotation_step = 0;
    int64_t now = esp_timer_get_time();


    xTaskCreatePinnedToCore( ble_recieve_parser, "ble_recieve_parser", 4096, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore( command_parser, "robot_command_parser", 4096, NULL, 5, NULL, 1);

    while (1) {
        /*
        if (gpio_get_level(IMU_INT_PIN) == 0) {
            while (gpio_get_level(IMU_INT_PIN) == 0) {
               // imu_check_safe(imu);
            }
        }
        */
        
        now = esp_timer_get_time();
        if (num_connected > 0) {
            if (now - last_send_time >= 5000000) { // in us
                send_health_report();
                switch (rotation_step) {
                    //case 0: send_imu(1); break;
                    //case 1: send_imu(2); break;
                    //case 2: send_imu(3); break;
                    //case 3: send_health_report(); break;
                }
                
                rotation_step = (rotation_step + 1) % 4; 

                last_send_time = now;
            }
        }
        
        //vTaskDelay(pdMS_TO_TICKS(1));
        
    }
    return;
}