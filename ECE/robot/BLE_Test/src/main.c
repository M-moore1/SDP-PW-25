
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "stepper_motor.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "driver/uart.h"

#include "nvs_flash.h"
#include "esp_log.h"
#include "Robot_BLE.h"
#include "pinout.h"
#include "robot_commands.h"
#include "i2c.h"
#include "imu.h"
//#include "aes_gcm_decrypt.h"

step_mot_t test_motor;
step_mot_t front_left;
step_mot_t front_right;
step_mot_t back_left;
step_mot_t back_right;

static QueueHandle_t cmd_queue = NULL;

void ble_recieve_parser(void *pvParameters)
{
    uint8_t received_packet[packet_size]; 
    char *msg1 = "Bytes Recieved";

    while (1) {
        if (xQueueReceive(ble_recieve_queue, &received_packet, portMAX_DELAY))
        {   
            send_string(msg1);
            printf("Received %d bytes: \n", packet_size);

            for (int i = 0; i < packet_size; i++) {
                printf("%02X ", received_packet[i]);
                if ((i + 1) % 16 == 0) printf("\n"); 
            }
            printf("\n");
            printf("End of Bytes");
            
            if (security_flag) {
                /*
                printf("Decrypting bytes...\n");
                char plaintext[256];
                size_t pt_len = 0;
                int ret = aes_gcm_decrypt_packet(received_packet, plaintext, &pt_len);
                if (ret == 0) {
                    printf("Plaintext: %s\n", plaintext);
                } else {
                    printf("Decryption failed: %d\n", ret);
                }
                */
            }

            if (xQueueSend(cmd_queue, received_packet, 0) != pdPASS) {
                ESP_LOGW(GATTS_TABLE_TAG, "cmd_queue full");
            }
        }
    }
}

void command_parser(void *pvParameters)
{
    uint64_t inst;
    robot_bt_packet_t cmd;
    while (1)
    {
        if (xQueueReceive(cmd_queue, &inst, portMAX_DELAY))
        {
            printf("Command Parse Initiated \n");
            cmd.raw = inst;

            command_type_t cmd_type = (command_type_t)cmd.ctrl.type;
            ESP_LOGI("CMD", "Extracted type: %u", cmd_type);

            switch (cmd_type){
                case CONTROL_CMD:
                    printf("I got a control instruction");
                    control_cmd(cmd.ctrl, &front_left, &front_right, &back_left, &back_right);
                break;

                case ARM_CMD:
                    printf("I got a Pose instruction");
                    arm_cmd(inst, &test_motor, &test_motor, &test_motor, &test_motor);
                break;

                case System_CMD:
                    printf("I got a System instruction");
                    system_cmd(cmd.sys, &test_motor, &test_motor, &test_motor, &test_motor);
                break;
                
                case Query_CMD:
                    printf("I got a Query Instruction");
                    query_cmd(cmd.query, &test_motor, &test_motor, &test_motor, &test_motor);
                break;
                
                default:
                    printf("Invalid Command");
                    ack_format_t ack = {0};
                    ack.pl = 1;
                    ack.type = ACK_CMD;       
                    ack.id = cmd.query.id;    
                    ack.result_code = RESULT_UNKNOWN_CMD;
                    printf("\nInvalid Command\n");
                break;
            }


            
        }
    }
}

void app_main()
{
    i2c_master_bus_handle_t i2c_bus = i2c_init();
    i2c_master_dev_handle_t imu     = device_init(i2c_bus, IMU_ADDR);

    if (!bno08x_init(imu)) {
        ESP_LOGE(IMU_TAG, "Failed to init BNO08x, halting");
        while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }

    ESP_ERROR_CHECK(nvs_flash_init()); // Initialize NVS
    robot_ble_init();                  // Initialize BLE

    motor_init(&front_left,  FL_MOTOR_STEP, FL_MOTOR_DIR, FL_MOTOR_EN, FL_MOTOR_PWM);
    motor_init(&front_right, FR_MOTOR_STEP, FR_MOTOR_DIR, FR_MOTOR_EN, FR_MOTOR_PWM);
    motor_init(&back_left,   BL_MOTOR_STEP, BL_MOTOR_DIR, BL_MOTOR_EN, BL_MOTOR_PWM);
    motor_init(&back_right,  BR_MOTOR_STEP, BR_MOTOR_DIR, BR_MOTOR_EN, BR_MOTOR_PWM);

    cmd_queue = xQueueCreate(10, sizeof(uint64_t)); // Initialize the command queue
    
    char *msg1 = "hello";
    char *msg2 = "testing";
    robot_bt_packet_t status_pkt = {0};
    status_pkt.nav.type  = 0x05;

    xTaskCreatePinnedToCore( ble_recieve_parser, "ble_recieve_parser", 4096, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore( command_parser, "robot_command_parser", 4096, NULL, 5, NULL, 1);

    while (1) {
        imu_check(imu);
        
        
        
        if (device_connected && notify_enabled) {
            /*
            status_pkt.nav.pl    = 1;
            status_pkt.nav.type  = ROBOT_UPDATE_CMD; 
            status_pkt.nav.part  = 0;                
            status_pkt.nav.speed = 50; 

            status_pkt.nav.pos_x = 0; // Signed mm
            status_pkt.nav.pos_y = 0; // Signed mm
            status_pkt.nav.pos_z = 0; // Signed mm
            send_instr(status_pkt.bytes);
            vTaskDelay(pdMS_TO_TICKS(10));

            status_pkt.pose.pl    = 1;
            status_pkt.pose.type  = ROBOT_UPDATE_CMD; // 0x05
            status_pkt.pose.part  = 1;                // 0b01

            status_pkt.pose.yaw   = (uint32_t)(g_imu_euler.yaw * 100);   // Unsigned
            status_pkt.pose.pitch = (int32_t)(g_imu_euler.pitch * 100);  // Signed
            status_pkt.pose.roll  = (int32_t)(g_imu_euler.roll * 100);   // Signed
            send_instr(status_pkt.bytes);
            vTaskDelay(pdMS_TO_TICKS(10));

            status_pkt.inert.pl   = 1;
            status_pkt.inert.type = ROBOT_UPDATE_CMD; // 0x05
            status_pkt.inert.part = 2;                // 0b10

            // Acceleration (m/s²) - Signed
            status_pkt.inert.accel_x = (int16_t)(g_imu_accel.x * 10);
            status_pkt.inert.accel_y = (int16_t)(g_imu_accel.y * 10);
            status_pkt.inert.accel_z = (int16_t)(g_imu_accel.z * 10);

            // Gyroscope (deg/s) - Signed
            status_pkt.inert.gyro_x  = (int16_t)(g_imu_gyro.x_degs * 10);
            status_pkt.inert.gyro_y  = (int16_t)(g_imu_gyro.y_degs * 10);
            status_pkt.inert.gyro_z  = (int16_t)(g_imu_gyro.z_degs * 10);
            send_instr(status_pkt.bytes);
            vTaskDelay(pdMS_TO_TICKS(10));
            
            send_string(msg2);  
            vTaskDelay(pdMS_TO_TICKS(200));
            send_string(msg1);
            */
        }
        
        vTaskDelay(pdMS_TO_TICKS(2000)); // 2 seconds
        
    }
    return;
}