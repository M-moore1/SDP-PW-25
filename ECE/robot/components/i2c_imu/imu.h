#ifndef imu_H
#define imu_H

#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include <string.h>
#include <math.h>
#include "pinout.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define IMU_TAG         "IMU"
#define IMU_ADDR        0x4A
#define SHTP_BUF_SIZE   512
#define R2D             (180.0f / (float)M_PI)
#define REPORT_ACCELEROMETER    0x01
#define REPORT_GYROSCOPE        0x02
#define REPORT_ROTATION_VECTOR  0x05
#define REPORT_SET_FEATURE      0xFD
#define REPORT_ID_OFFSET  13
#define DATA_OFFSET       17

typedef struct { 
    float i, j, k, real, accuracy_rad; 
} quaternion_t;

typedef struct {
    float x, y, z;
} vec3_t;

typedef struct {
    float x_rads, y_rads, z_rads;  // rad/s
    float x_degs, y_degs, z_degs;  // deg/s
} gyro_t;

typedef struct { 
    float yaw, pitch, roll; 
} euler_t;



extern quaternion_t g_imu_quat;
extern vec3_t       g_imu_accel;
extern gyro_t       g_imu_gyro;
extern euler_t      g_imu_euler;

size_t shtp_read(i2c_master_dev_handle_t dev, uint8_t *buf, size_t buf_len);
esp_err_t shtp_write(i2c_master_dev_handle_t dev, uint8_t channel, 
                                uint8_t *payload, size_t payload_len);
esp_err_t bno08x_enable_report(i2c_master_dev_handle_t dev,
                                       uint8_t report_id, uint32_t interval_us);
bool bno08x_init(i2c_master_dev_handle_t dev);
int imu_check(i2c_master_dev_handle_t imu);

#endif