
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
uint8_t payload[156] = {

    0x8B, 0xDF, 0x57, 0x3D, 0x3D, 0x50, 0xAF, 0x81, 0xFA, 0xD2, 0x9D, 0x95, 0x5B, 0x91, 0xD6, 0xBC,

    0x09, 0xFF, 0x99, 0x70, 0x95, 0x65, 0xF1, 0x14, 0xAC, 0xDA, 0x91, 0x46, 0x93, 0x79, 0xB8, 0x6E,

    0xC6, 0x38, 0x24, 0xB3, 0x73, 0x39, 0xEE, 0xC7, 0xAA, 0x90, 0x39, 0x29, 0x13, 0x7B, 0xD3, 0x67,

    0xD0, 0x0B, 0x0A, 0x99, 0x53, 0x45, 0x05, 0xFB, 0x3E, 0x60, 0x3E, 0x24, 0xD0, 0x2D, 0x77, 0xB1,

    0xDB, 0xEC, 0xA6, 0x93, 0x1B, 0x98, 0x1D, 0x18, 0xAE, 0x31, 0x6F, 0xA1, 0x4F, 0x2B, 0x32, 0xB8,

    0xCB, 0x77, 0x83, 0x05, 0x17, 0x4D, 0x69, 0x61, 0xFD, 0xD3, 0xBE, 0xA0, 0xBE, 0x07, 0x12, 0x23,

    0xC7, 0x5C, 0x38, 0x56, 0x28, 0x67, 0x88, 0xAA, 0xEA, 0x14, 0xBC, 0xCD, 0x93, 0xA3, 0xF7, 0x6F,

    0xD1, 0xDD, 0xDD, 0x58, 0xC8, 0x24, 0xE3, 0x46, 0x7B, 0x73, 0xFF, 0x81, 0xF4, 0xD4, 0xAF, 0xAD,

    0x61, 0xDB, 0x43, 0x98, 0xF1, 0x19, 0x54, 0x2F, 0x13, 0x6E, 0x2C, 0x6A, 0xAB, 0x3B, 0x68, 0x61,

    0x5B, 0xCE, 0x9D, 0x26, 0xE1, 0x48, 0xF0, 0x42, 0x26, 0xC4, 0x99, 0x4C,

};

void ble_recieve_parser(void *pvParameters)
{
    uint8_t received_packet[packet_size]; 
    char *msg1 = "Bytes Recieved";
    robot_bt_packet_t return_msg;

    uint8_t data[8] = { 0x12, 0x34, 0x56, 0x78, 0x54, 0x23, 0x08, 0x04 };
    memcpy(return_msg.bytes, data, 8);

    while (1) {
        if (xQueueReceive(ble_recieve_queue, &received_packet, portMAX_DELAY))
        {   
            // SEND 8 bytes
            // send_bytes(data, 8);
            // SEND 156 Bytes
            send_bytes(payload, 156);
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
        //imu_check(imu);
        
        
        
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