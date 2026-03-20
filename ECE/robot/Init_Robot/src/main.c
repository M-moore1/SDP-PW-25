#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>
#include <math.h>

#include "stepper_motor.h"
#include "robot_bt.h"
#include "pinout.h"
#include "robot_commands.h"
#include "i2c.h"
#include "imu.h"

#define PRINT_EVERY_MS  500

step_mot_t front_left;
step_mot_t front_right;
step_mot_t back_left;
step_mot_t back_right;


void app_main(void) {

    i2c_master_bus_handle_t i2c_bus = i2c_init();
    i2c_master_dev_handle_t imu     = device_init(i2c_bus, IMU_ADDR);

    if (!bno08x_init(imu)) {
        ESP_LOGE(IMU_TAG, "Failed to init BNO08x, halting");
        while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }

    motor_init(&front_left,  FL_MOTOR_STEP, FL_MOTOR_DIR, FL_MOTOR_EN, FL_MOTOR_PWM);
    motor_init(&front_right, FR_MOTOR_STEP, FR_MOTOR_DIR, FR_MOTOR_EN, FR_MOTOR_PWM);
    motor_init(&back_left,   BL_MOTOR_STEP, BL_MOTOR_DIR, BL_MOTOR_EN, BL_MOTOR_PWM);
    motor_init(&back_right,  BR_MOTOR_STEP, BR_MOTOR_DIR, BR_MOTOR_EN, BR_MOTOR_PWM);

    

    int64_t last_print_us = esp_timer_get_time();

    while (1) {
       imu_check(imu);

        // Print at throttled rate
        int64_t now_us = esp_timer_get_time();
        if (now_us - last_print_us >= PRINT_EVERY_MS * 1000LL) {
            last_print_us = now_us;

            ESP_LOGI(IMU_TAG, "ACCEL  x=%7.3f  y=%7.3f  z=%7.3f m/s2",
                     g_imu_accel.x, g_imu_accel.y, g_imu_accel.z);

            ESP_LOGI(IMU_TAG, "GYRO   x=%7.3f  y=%7.3f  z=%7.3f rad/s  |  x=%7.2f  y=%7.2f  z=%7.2f deg/s",
                     g_imu_gyro.x_rads, g_imu_gyro.y_rads, g_imu_gyro.z_rads,
                     g_imu_gyro.x_degs, g_imu_gyro.y_degs, g_imu_gyro.z_degs);

            ESP_LOGI(IMU_TAG, "EULER  yaw=%7.2f  pitch=%7.2f  roll=%7.2f deg",
                     g_imu_euler.yaw, g_imu_euler.pitch, g_imu_euler.roll);
            
        }

        vTaskDelay(pdMS_TO_TICKS(5));
    }
}