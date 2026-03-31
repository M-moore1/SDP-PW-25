#include "imu.h"

quaternion_t g_imu_quat  = {0};
vec3_t       g_imu_accel = {0};
gyro_t       g_imu_gyro  = {0};
euler_t      g_imu_euler = {0};


size_t shtp_read(i2c_master_dev_handle_t dev, uint8_t *buf, size_t buf_len) {
    uint8_t header[4] = {0};
    vTaskDelay(pdMS_TO_TICKS(2));
    if (i2c_master_receive(dev, header, 4, 9000) != ESP_OK) return 0;

    uint16_t packet_len = ((uint16_t)header[1] << 8 | header[0]) & ~0x8000;
    if (packet_len == 0 || packet_len > buf_len - 4) return 0;

    memcpy(buf, header, 4);
    vTaskDelay(pdMS_TO_TICKS(2));
    if (i2c_master_receive(dev, buf + 4, packet_len, 9000) != ESP_OK) return 0;

    return packet_len;
}

esp_err_t shtp_write(i2c_master_dev_handle_t dev, uint8_t channel, uint8_t *payload, size_t payload_len) {
    size_t total = payload_len + 4;
    uint8_t buf[total];
    buf[0] = total & 0xFF;
    buf[1] = (total >> 8) & 0xFF;
    buf[2] = channel;
    buf[3] = 0;
    memcpy(buf + 4, payload, payload_len);
    return i2c_master_transmit(dev, buf, total, 5000);
}

esp_err_t bno08x_enable_report(i2c_master_dev_handle_t dev, uint8_t report_id, uint32_t interval_us) {
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

void imu_hard_reset(){
    gpio_set_level(IMU_RESET_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(10));

    gpio_set_level(IMU_RESET_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(500));
}

// Drain packets until INT goes high (no more data)
void drain_packets(i2c_master_dev_handle_t dev) {
    uint8_t buf[SHTP_BUF_SIZE];
    for (int i = 0; i < 10; i++) {
        if (gpio_get_level(IMU_INT_PIN) == 1) break;  // INT high = no data
        shtp_read(dev, buf, sizeof(buf));
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

bool bno08x_init(i2c_master_dev_handle_t dev) {
    gpio_set_direction(IMU_INT_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(IMU_RESET_PIN, GPIO_MODE_OUTPUT);

    imu_hard_reset();

    // Soft reset via SHTP channel 1
    uint8_t reset_payload[] = {0x01};
    bool reset_ok = false;
    for (int i = 0; i < 5; i++) {
        if (shtp_write(dev, 1, reset_payload, sizeof(reset_payload)) == ESP_OK) {
            reset_ok = true;
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    if (!reset_ok) { ESP_LOGE(IMU_TAG, "Soft reset failed"); return false; }

    // Wait for boot
    vTaskDelay(pdMS_TO_TICKS(500));

    // Drain startup packets
    drain_packets(dev);

    ESP_LOGI(IMU_TAG, "BNO08x ready");

    // Enable reports at 50Hz
    bno08x_enable_report(dev, REPORT_ACCELEROMETER,   20000);
    bno08x_enable_report(dev, REPORT_GYROSCOPE,       20000);
    bno08x_enable_report(dev, REPORT_ROTATION_VECTOR, 20000);

    ESP_LOGI(IMU_TAG, "IMU init and reports enabled");
    return true;
}

static uint8_t g_tare_seq_num = 0;
inline int16_t read_i16(uint8_t *buf, int offset) {
    return (int16_t)((uint16_t)buf[offset] | (uint16_t)buf[offset + 1] << 8);
}

// Tare Now — zeros the orientation on the specified basis
bool bno085_tare_cmd(i2c_master_dev_handle_t dev) {
    uint8_t payload[12] = {0}; // Zero-initialize all 12 bytes
    
    payload[0] = 0xF2;              // Report ID: Command Request
    payload[1] = 0x00;  // Incrementing Sequence number
    payload[2] = 0x03;              // Command: Tare
    payload[3] = 0x00;              // P0: Subcommand (Tare Now)
    payload[4] = 0x07;              // P1: all 3 axes (X|Y|Z)
    payload[5] = 0x00;              // P2: Basis (0x00 = Rotation Vector)
    // Bytes 6-11 are 0x00

    if (shtp_write(dev, 2, payload, 12) == ESP_OK) {
        ESP_LOGI(IMU_TAG, "Tare Now sent (Seq %d)", payload[1]);
        vTaskDelay(pdMS_TO_TICKS(150)); 
        drain_packets(dev);             
        return true;
    }
    
    ESP_LOGE(IMU_TAG, "Failed to send Tare Now");
    return false;
}

// Persist Tare — saves tare to flash
bool bno085_save_settings_cmd(i2c_master_dev_handle_t dev) {
    uint8_t payload[12] = {0}; // Zero-initialize all 12 bytes

    payload[0] = 0xF2;              // Report ID: Command Request
    payload[1] = 0x00;  // Incrementing Sequence number
    payload[2] = 0x03;              // Command: Tare
    payload[3] = 0x01;              // P0: Subcommand (Persist Tare)
    // Bytes 4-11 are 0x00 (Reserved)

    if (shtp_write(dev, 2, payload, 12) == ESP_OK) {
        ESP_LOGI(IMU_TAG, "Persist Tare sent (Seq %d)", payload[1]);
        vTaskDelay(pdMS_TO_TICKS(200)); // Flash persistence takes longer
        drain_packets(dev);
        return true;
    }

    ESP_LOGE(IMU_TAG, "Failed to send Persist Tare");
    return false;
}

bool bno085_tare_yaw_only_cmd(i2c_master_dev_handle_t dev) {
    uint8_t payload[12] = {0};
    payload[0] = 0xF2;
    payload[1] = g_tare_seq_num++;
    payload[2] = 0x03; // Command: Tare
    payload[3] = 0x00; // P0: Tare Now
    payload[4] = 0x04; // P1: ONLY Z-axis (as per your datasheet find)
    payload[5] = 0x00; // P2: Rotation Vector Basis
    
    return (shtp_write(dev, 2, payload, 12) == ESP_OK);
}

// Tare Roll Only (X-axis)
bool bno085_tare_roll_only_cmd(i2c_master_dev_handle_t dev) {
    uint8_t payload[12] = {0};
    payload[0] = 0xF2;
    payload[1] = g_tare_seq_num++;
    payload[2] = 0x03; // Command: Tare
    payload[3] = 0x00; // P0: Tare Now
    payload[4] = 0x01; // P1: ONLY X-axis (Roll)
    payload[5] = 0x00; // P2: Rotation Vector Basis
    
    if (shtp_write(dev, 2, payload, 12) == ESP_OK) {
        vTaskDelay(pdMS_TO_TICKS(100));
        drain_packets(dev);
        return true;
    }
    return false;
}

// Tare Pitch Only (Y-axis)
bool bno085_tare_pitch_only_cmd(i2c_master_dev_handle_t dev) {
    uint8_t payload[12] = {0};
    payload[0] = 0xF2;
    payload[1] = g_tare_seq_num++;
    payload[2] = 0x03; // Command: Tare
    payload[3] = 0x00; // P0: Tare Now
    payload[4] = 0x02; // P1: ONLY Y-axis (Pitch)
    payload[5] = 0x00; // P2: Rotation Vector Basis
    
    if (shtp_write(dev, 2, payload, 12) == ESP_OK) {
        vTaskDelay(pdMS_TO_TICKS(100));
        drain_packets(dev);
        return true;
    }
    return false;
}


static bool parse_accelerometer(uint8_t *buf, size_t len, vec3_t *out) {
    if ((int)len + 4 < DATA_OFFSET + 6) return false;
    if (buf[REPORT_ID_OFFSET] != REPORT_ACCELEROMETER) return false;
    const float scale = 1.0f / 256.0f;
    out->x = read_i16(buf, DATA_OFFSET + 0) * scale;
    out->y = read_i16(buf, DATA_OFFSET + 2) * scale;
    out->z = read_i16(buf, DATA_OFFSET + 4) * scale;
    return true;
}

static bool parse_gyroscope(uint8_t *buf, size_t len, gyro_t *out) {
    if ((int)len + 4 < DATA_OFFSET + 6) return false;
    if (buf[REPORT_ID_OFFSET] != REPORT_GYROSCOPE) return false;
    const float scale = 1.0f / 512.0f;
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
    const float scale_q   = 1.0f / 16384.0f;
    const float scale_acc = 1.0f / 4096.0f;
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
    e.roll  = atan2f(sinr, cosr) * R2D;
    float sinp = 2.0f * (q->real * q->j - q->k * q->i);
    e.pitch = (fabsf(sinp) >= 1.0f) ? copysignf(90.0f, sinp) : asinf(sinp) * R2D;
    float siny = 2.0f * (q->real * q->k + q->i * q->j);
    float cosy = 1.0f - 2.0f * (q->j * q->j + q->k * q->k);
    e.yaw   = atan2f(siny, cosy) * R2D;
    return e;
}

int imu_check(i2c_master_dev_handle_t imu) {
    static uint8_t last_seq = 0xFF;
    uint8_t buf[SHTP_BUF_SIZE];

    if (gpio_get_level(IMU_INT_PIN) == 1) return -1;  // No data ready

    size_t len = shtp_read(imu, buf, sizeof(buf));
    if (len == 0) return -1;
    //if (buf[3] == last_seq) return -1;
    last_seq = buf[3];

    if      (parse_accelerometer(buf, len, &g_imu_accel))  {}
    else if (parse_gyroscope(buf, len, &g_imu_gyro))       {}
    else if (parse_rotation_vector(buf, len, &g_imu_quat)) {
        g_imu_euler = quaternion_to_euler(&g_imu_quat);
    }
    return 0;
}

static int imu_error_count = 0;

void imu_check_safe(i2c_master_dev_handle_t imu) {
    int ret = imu_check(imu);   
    if (ret != ESP_OK) {
        imu_error_count++;
        if (imu_error_count >= IMU_MAX_ERROR) {
            ESP_LOGE(IMU_TAG, "IMU ERROR — resetting BNO08x");
            bno08x_init(imu);
            imu_error_count = 0;
        }
    } else {
        imu_error_count = 0; 
    }
}

void imu_tare_process(i2c_master_dev_handle_t imu){
    while (gpio_get_level(IMU_INT_PIN) == 1) { vTaskDelay(pdMS_TO_TICKS(1)); }
    if (!bno085_tare_cmd(imu)) {
    ESP_LOGE(IMU_TAG, "Failed to tare, halting");
    while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }

    // READ a 1-2 packets
    drain_packets(imu);


    // Persist Tare
    while (gpio_get_level(IMU_INT_PIN) == 1) { vTaskDelay(pdMS_TO_TICKS(1)); }
    if (!bno085_save_settings_cmd(imu)) {
    ESP_LOGE(IMU_TAG, "Failed to persist tare, halting");
    while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }

    bno085_tare_cmd(imu);
    drain_packets(imu);
    bno085_save_settings_cmd(imu);
    ESP_LOGI(IMU_TAG, "IMU Tared");
}