#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>
#include <math.h>

#include "stepper_motor.h"
#include "robot_bt.h"
#include "pinout.h"
#include "robot_commands.h"

static const char *BNO_TAG = "BNO08x";

#define IMU_ADDR        0x4A
#define SHTP_BUF_SIZE   512
#define PRINT_EVERY_MS  500
#define R2D             (180.0f / (float)M_PI)

#define REPORT_ACCELEROMETER    0x01
#define REPORT_GYROSCOPE        0x02
#define REPORT_ROTATION_VECTOR  0x05
#define REPORT_SET_FEATURE      0xFD

// Confirmed byte offsets (verified from raw packet dumps + gravity check):
//   buf[0..3]   SHTP header (len_lsb, len_msb, channel, seq)
//   buf[4..7]   SHTP payload header
//   buf[8]      0xFB timebase reference ID
//   buf[9..12]  base delta timestamp (4 bytes)
//   buf[13]     sensor report ID
//   buf[14]     sequence, [15] status, [16] delay
//   buf[17..]   sensor data
//
// Q points (datasheet + verified):
//   Accel  Q8  -> 1/256     m/s²   (z=9.758 flat)
//   Gyro   Q9  -> 1/512     rad/s  (0.000 still)
//   Quat   Q14 -> 1/16384   [-1,1] (|q|=1.0000)
//   Acc    Q12 -> 1/4096    radians

#define REPORT_ID_OFFSET  13
#define DATA_OFFSET       17

// ----------------------------------------------------------------
// I2C init
// ----------------------------------------------------------------

i2c_master_bus_handle_t i2c_init() {
    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = GPIO_NUM_22,
        .sda_io_num = GPIO_NUM_21,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));
    return bus_handle;
}

i2c_master_dev_handle_t device_init(i2c_master_bus_handle_t bus_handle, uint8_t dev_addr) {
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = dev_addr,
        .scl_speed_hz = 50000,
        .scl_wait_us = 1000,
    };
    i2c_master_dev_handle_t dev_handle;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));
    return dev_handle;
}

// ----------------------------------------------------------------
// SHTP read / write
// ----------------------------------------------------------------

static size_t shtp_read(i2c_master_dev_handle_t dev, uint8_t *buf, size_t buf_len) {
    uint8_t header[4] = {0};
    vTaskDelay(pdMS_TO_TICKS(2));
    if (i2c_master_receive(dev, header, 4, 5000) != ESP_OK) return 0;

    uint16_t packet_len = ((uint16_t)header[1] << 8 | header[0]) & ~0x8000;
    if (packet_len == 0 || packet_len > buf_len - 4) return 0;

    memcpy(buf, header, 4);
    vTaskDelay(pdMS_TO_TICKS(2));
    if (i2c_master_receive(dev, buf + 4, packet_len, 5000) != ESP_OK) return 0;

    return packet_len;
}

static esp_err_t shtp_write(i2c_master_dev_handle_t dev, uint8_t channel,
                             uint8_t *payload, size_t payload_len) {
    size_t total = payload_len + 4;
    uint8_t buf[total];
    buf[0] = total & 0xFF;
    buf[1] = (total >> 8) & 0xFF;
    buf[2] = channel;
    buf[3] = 0;
    memcpy(buf + 4, payload, payload_len);
    return i2c_master_transmit(dev, buf, total, 5000);
}

// ----------------------------------------------------------------
// BNO08x commands
// ----------------------------------------------------------------

static esp_err_t bno08x_enable_report(i2c_master_dev_handle_t dev,
                                       uint8_t report_id, uint32_t interval_us) {
    uint8_t payload[] = {
        REPORT_SET_FEATURE, report_id, 0x00, 0x00, 0x00,
        (interval_us >>  0) & 0xFF,
        (interval_us >>  8) & 0xFF,
        (interval_us >> 16) & 0xFF,
        (interval_us >> 24) & 0xFF,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };
    return shtp_write(dev, 2, payload, sizeof(payload));
}

static bool bno08x_init(i2c_master_dev_handle_t dev) {
    uint8_t reset_pkt[] = {5, 0, 1, 0, 1};
    bool reset_ok = false;
    for (int i = 0; i < 5; i++) {
        if (i2c_master_transmit(dev, reset_pkt, sizeof(reset_pkt), 5000) == ESP_OK) {
            reset_ok = true;
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    if (!reset_ok) { ESP_LOGE(BNO_TAG, "Soft reset failed"); return false; }
    vTaskDelay(pdMS_TO_TICKS(500));

    uint8_t buf[SHTP_BUF_SIZE];
    for (int i = 0; i < 3; i++) shtp_read(dev, buf, sizeof(buf));

    ESP_LOGI(BNO_TAG, "BNO08x ready");
    return true;
}

// ----------------------------------------------------------------
// Data types
// ----------------------------------------------------------------

typedef struct { float i, j, k, real, accuracy_rad; } quaternion_t;

typedef struct {
    float x, y, z;
} vec3_t;

typedef struct {
    float x_rads, y_rads, z_rads;  // rad/s
    float x_degs, y_degs, z_degs;  // deg/s
} gyro_t;

typedef struct { float yaw, pitch, roll; } euler_t;

// Public IMU state - accessible from other files
quaternion_t g_imu_quat  = {0};
vec3_t       g_imu_accel = {0};
gyro_t       g_imu_gyro  = {0};
euler_t      g_imu_euler = {0};

// ----------------------------------------------------------------
// Parsers
// ----------------------------------------------------------------

static inline int16_t read_i16(uint8_t *buf, int offset) {
    return (int16_t)((uint16_t)buf[offset] | (uint16_t)buf[offset + 1] << 8);
}

static bool parse_accelerometer(uint8_t *buf, size_t len, vec3_t *out) {
    if ((int)len + 4 < DATA_OFFSET + 6) return false;
    if (buf[REPORT_ID_OFFSET] != REPORT_ACCELEROMETER) return false;
    const float scale = 1.0f / 256.0f; // Q8 -> m/s²
    out->x = read_i16(buf, DATA_OFFSET + 0) * scale;
    out->y = read_i16(buf, DATA_OFFSET + 2) * scale;
    out->z = read_i16(buf, DATA_OFFSET + 4) * scale;
    return true;
}

static bool parse_gyroscope(uint8_t *buf, size_t len, gyro_t *out) {
    if ((int)len + 4 < DATA_OFFSET + 6) return false;
    if (buf[REPORT_ID_OFFSET] != REPORT_GYROSCOPE) return false;
    const float scale = 1.0f / 512.0f; // Q9 -> rad/s
    out->x_rads = read_i16(buf, DATA_OFFSET + 0) * scale;
    out->y_rads = read_i16(buf, DATA_OFFSET + 2) * scale;
    out->z_rads = read_i16(buf, DATA_OFFSET + 4) * scale;
    out->x_degs = out->x_rads * R2D;
    out->y_degs = out->y_rads * R2D;
    out->z_degs = out->z_rads * R2D;
    return true;
}

static bool parse_rotation_vector(uint8_t *buf, size_t len, quaternion_t *out) {
    if ((int)len + 4 < DATA_OFFSET + 10) return false;
    if (buf[REPORT_ID_OFFSET] != REPORT_ROTATION_VECTOR) return false;
    const float scale_q   = 1.0f / 16384.0f; // Q14 -> [-1,1]
    const float scale_acc = 1.0f / 4096.0f;  // Q12 -> radians
    out->i            = read_i16(buf, DATA_OFFSET + 0) * scale_q;
    out->j            = read_i16(buf, DATA_OFFSET + 2) * scale_q;
    out->k            = read_i16(buf, DATA_OFFSET + 4) * scale_q;
    out->real         = read_i16(buf, DATA_OFFSET + 6) * scale_q;
    out->accuracy_rad = read_i16(buf, DATA_OFFSET + 8) * scale_acc;
    return true;
}

// ----------------------------------------------------------------
// Quaternion -> Euler angles (degrees)
// ----------------------------------------------------------------

static euler_t quaternion_to_euler(const quaternion_t *q) {
    euler_t e;

    float sinr = 2.0f * (q->real * q->i + q->j * q->k);
    float cosr = 1.0f - 2.0f * (q->i * q->i + q->j * q->j);
    e.roll = atan2f(sinr, cosr) * R2D;

    float sinp = 2.0f * (q->real * q->j - q->k * q->i);
    e.pitch = (fabsf(sinp) >= 1.0f) ? copysignf(90.0f, sinp) : asinf(sinp) * R2D;

    float siny = 2.0f * (q->real * q->k + q->i * q->j);
    float cosy = 1.0f - 2.0f * (q->j * q->j + q->k * q->k);
    e.yaw = atan2f(siny, cosy) * R2D;

    return e;
}

// ----------------------------------------------------------------
// Motors
// ----------------------------------------------------------------

step_mot_t front_left;
step_mot_t front_right;
step_mot_t back_left;
step_mot_t back_right;

// ----------------------------------------------------------------
// app_main
// ----------------------------------------------------------------

void app_main(void) {

    i2c_master_bus_handle_t i2c_bus = i2c_init();
    i2c_master_dev_handle_t imu     = device_init(i2c_bus, IMU_ADDR);

    if (!bno08x_init(imu)) {
        ESP_LOGE(BNO_TAG, "Failed to init BNO08x, halting");
        while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }

    motor_init(&front_left,  FL_MOTOR_STEP, FL_MOTOR_DIR, FL_MOTOR_EN, FL_MOTOR_PWM);
    motor_init(&front_right, FR_MOTOR_STEP, FR_MOTOR_DIR, FR_MOTOR_EN, FR_MOTOR_PWM);
    motor_init(&back_left,   BL_MOTOR_STEP, BL_MOTOR_DIR, BL_MOTOR_EN, BL_MOTOR_PWM);
    motor_init(&back_right,  BR_MOTOR_STEP, BR_MOTOR_DIR, BR_MOTOR_EN, BR_MOTOR_PWM);

    bno08x_enable_report(imu, REPORT_ACCELEROMETER,   20000);  // 50Hz
    bno08x_enable_report(imu, REPORT_GYROSCOPE,       20000);
    bno08x_enable_report(imu, REPORT_ROTATION_VECTOR, 20000);

    uint8_t buf[SHTP_BUF_SIZE];
    size_t  len;
    int64_t last_print_us = esp_timer_get_time();
    uint8_t last_seq      = 0xFF;

    while (1) {
        len = shtp_read(imu, buf, sizeof(buf));

        if (len == 0) {
            vTaskDelay(pdMS_TO_TICKS(5));
            continue;
        }

        // Skip duplicate packets
        if (buf[3] == last_seq) {
            vTaskDelay(pdMS_TO_TICKS(5));
            continue;
        }
        last_seq = buf[3];

        // Parse into globals
        if      (parse_accelerometer(buf, len, &g_imu_accel))    {}
        else if (parse_gyroscope(buf, len, &g_imu_gyro))         {}
        else if (parse_rotation_vector(buf, len, &g_imu_quat))   {
            g_imu_euler = quaternion_to_euler(&g_imu_quat);
        }

        // Print at throttled rate
        int64_t now_us = esp_timer_get_time();
        if (now_us - last_print_us >= PRINT_EVERY_MS * 1000LL) {
            last_print_us = now_us;

            ESP_LOGI(BNO_TAG, "ACCEL  x=%7.3f  y=%7.3f  z=%7.3f m/s2",
                     g_imu_accel.x, g_imu_accel.y, g_imu_accel.z);

            ESP_LOGI(BNO_TAG, "GYRO   x=%7.3f  y=%7.3f  z=%7.3f rad/s  |  x=%7.2f  y=%7.2f  z=%7.2f deg/s",
                     g_imu_gyro.x_rads, g_imu_gyro.y_rads, g_imu_gyro.z_rads,
                     g_imu_gyro.x_degs, g_imu_gyro.y_degs, g_imu_gyro.z_degs);

            ESP_LOGI(BNO_TAG, "EULER  yaw=%7.2f  pitch=%7.2f  roll=%7.2f deg",
                     g_imu_euler.yaw, g_imu_euler.pitch, g_imu_euler.roll);

            ESP_LOGI(BNO_TAG, "---");
        }

        vTaskDelay(pdMS_TO_TICKS(5));
    }
}