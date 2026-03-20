#include "imu.h"

quaternion_t g_imu_quat  = {0};
vec3_t       g_imu_accel = {0};
gyro_t       g_imu_gyro  = {0};
euler_t      g_imu_euler = {0};

size_t shtp_read(i2c_master_dev_handle_t dev, uint8_t *buf, size_t buf_len) {
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

esp_err_t shtp_write(i2c_master_dev_handle_t dev, uint8_t channel, 
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

esp_err_t bno08x_enable_report(i2c_master_dev_handle_t dev,
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

bool bno08x_init(i2c_master_dev_handle_t dev) {
    uint8_t reset_pkt[] = {5, 0, 1, 0, 1};
    bool reset_ok = false;
    for (int i = 0; i < 5; i++) {
        if (i2c_master_transmit(dev, reset_pkt, sizeof(reset_pkt), 5000) == ESP_OK) {
            reset_ok = true;
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    if (!reset_ok) { ESP_LOGE(IMU_TAG, "Soft reset failed"); return false; }
    vTaskDelay(pdMS_TO_TICKS(500));

    uint8_t buf[SHTP_BUF_SIZE];
    for (int i = 0; i < 3; i++) shtp_read(dev, buf, sizeof(buf));

    ESP_LOGI(IMU_TAG, "BNO08x ready");

    bno08x_enable_report(dev, REPORT_ACCELEROMETER,   20000);  // 50Hz
    bno08x_enable_report(dev, REPORT_GYROSCOPE,       20000);
    bno08x_enable_report(dev, REPORT_ROTATION_VECTOR, 20000);

    ESP_LOGI(IMU_TAG, "BNO08x accelerometer, gyroscope and rotation vector enabled");
    return true;
}

inline int16_t read_i16(uint8_t *buf, int offset) {
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

int imu_check(i2c_master_dev_handle_t imu){
    static uint8_t last_seq = 0xFF; 
    uint8_t buf[SHTP_BUF_SIZE];
    size_t  len;

    len = shtp_read(imu, buf, sizeof(buf));

    if (len == 0) {
        vTaskDelay(pdMS_TO_TICKS(5));
        return -1;
    }

    if (buf[3] == last_seq) {
        return -1;
    }
    last_seq = buf[3];

    if      (parse_accelerometer(buf, len, &g_imu_accel))    {}
    else if (parse_gyroscope(buf, len, &g_imu_gyro))         {}
    else if (parse_rotation_vector(buf, len, &g_imu_quat))   {
        g_imu_euler = quaternion_to_euler(&g_imu_quat);
    }

    return 0;

}