#include "arm.h"
#include "esp_log.h"
#include <math.h>

static const char* ARM_TAG = "ARM";

// LEDC timer (50 Hz, 14-bit)
static void arm_pwm_timer_init(void) {
    ledc_timer_config_t arm_timer = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_14_BIT,   // 16,384 steps
        .timer_num       = LEDC_TIMER_0,
        .freq_hz         = 50,                   // Standard servo frequency
        .clk_cfg         = LEDC_AUTO_CLK
    };
    ledc_timer_config(&arm_timer);
}

// Servo instances 
//  { pin, link_length, pwm_offset_us, current_angle, target_angle, pwm, channel }
static servo_t servo_base     = {16, ARM_D1,  550,  45, 45, 1000, LEDC_CHANNEL_4};
static servo_t servo_shoulder = {17, ARM_A2,  500,  45, 45, 1000, LEDC_CHANNEL_5};
static servo_t servo_elbow    = {18, ARM_A3, 1120,  45, 45, 1120, LEDC_CHANNEL_6};

// Current arm position 

static float arm_x = ARM_HOME_X;
static float arm_y = ARM_HOME_Y;
static float arm_z = ARM_HOME_Z;

// Internal helpers 
static float sin_d(float deg)          { return sinf(deg / 180.0f * (float)M_PI); }
static float cos_d(float deg)          { return cosf(deg / 180.0f * (float)M_PI); }
static float atan2_d(float y, float x) { return atan2f(y, x) / (float)M_PI * 180.0f; }
static float acos_d(float v)           { return acosf(v)      / (float)M_PI * 180.0f; }

static void servo_init_channel(servo_t *s) {
    ledc_channel_config_t cfg = {
        .gpio_num   = s->servo_pin,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = s->channel,
        .intr_type  = LEDC_INTR_DISABLE,
        .timer_sel  = LEDC_TIMER_0,
        .duty       = 0,
        .hpoint     = 0
    };
    ledc_channel_config(&cfg);
}

// Writes an angle directly to hardware — called every time a valid move comes in.
static void servo_write(servo_t *s, float angle) {
    s->current_angle = angle;
    s->target_angle  = angle;   // keep in sync — no interpolation in this version
    s->pwm           = angle * (10.0f / 9.0f) + (float)s->pwm_offset_us;
    uint32_t duty    = (uint32_t)(s->pwm * 16384.0f / 20000.0f);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, s->channel, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, s->channel);
}


int arm_ik_solve(float x, float y, float z, float servo_angles[3]) {
    float theta[3];

    // θ0 — base yaw from XY plane
    theta[0] = atan2_d(y, x);

    // Project onto the rotated XZ plane for 2-D solve
    float xr = cos_d(theta[0]) * x + sin_d(theta[0]) * y;
    float zr = z - ARM_D1;

    // θ1 — shoulder: elevation angle + law of cosines
    float r2   = xr * xr + zr * zr;
    float r    = sqrtf(r2);
    float cos1 = (r2 + ARM_A2 * ARM_A2 - ARM_A3 * ARM_A3) / (2.0f * r * ARM_A2);

    if (cos1 < -1.0f || cos1 > 1.0f) {
        ESP_LOGW(ARM_TAG, "IK: shoulder acos domain error (cos=%.3f)", cos1);
        return 1;
    }
    theta[1] = atan2_d(zr, xr) + acos_d(cos1);

    // θ2 — elbow: interior angle via law of cosines (complement form)
    float cos2 = (ARM_A3 * ARM_A3 + ARM_A2 * ARM_A2 - r2) / (2.0f * ARM_A2 * ARM_A3);

    if (cos2 < -1.0f || cos2 > 1.0f) {
        ESP_LOGW(ARM_TAG, "IK: elbow acos domain error (cos=%.3f)", cos2);
        return 1;
    }
    theta[2] = 180.0f - acos_d(cos2);

    // Joint limit checks
    if (theta[0] < ARM_THETA0_MIN || theta[0] > ARM_THETA0_MAX) {
        ESP_LOGW(ARM_TAG, "IK: base %.1f° out of [%.0f, %.0f]",
                 theta[0], ARM_THETA0_MIN, ARM_THETA0_MAX);
        return 1;
    }
    if (theta[1] < ARM_THETA1_MIN || theta[1] > ARM_THETA1_MAX) {
        ESP_LOGW(ARM_TAG, "IK: shoulder %.1f° out of [%.0f, %.0f]",
                 theta[1], ARM_THETA1_MIN, ARM_THETA1_MAX);
        return 1;
    }
    if (theta[2] < ARM_THETA2_MIN || theta[2] > ARM_THETA2_MAX) {
        ESP_LOGW(ARM_TAG, "IK: elbow %.1f° out of [%.0f, %.0f]",
                 theta[2], ARM_THETA2_MIN, ARM_THETA2_MAX);
        return 1;
    }

    servo_angles[0] = theta[0];
    servo_angles[1] = theta[1];
    servo_angles[2] = theta[2];
    return 0;
}

int arm_move_to(float x, float y, float z) {
    float angles[3];

    if (arm_ik_solve(x, y, z, angles) != 0) {
        ESP_LOGW(ARM_TAG, "Move rejected — IK failed for (%.2f, %.2f, %.2f)", x, y, z);
        return 1;
    }

    arm_x = x;
    arm_y = y;
    arm_z = z;

    servo_write(&servo_base,     angles[0]);
    servo_write(&servo_shoulder, angles[1]);
    servo_write(&servo_elbow,    angles[2]);

    ESP_LOGI(ARM_TAG, "Moved to (%.2f, %.2f, %.2f) | θ: %.1f° %.1f° %.1f°",
             arm_x, arm_y, arm_z, angles[0], angles[1], angles[2]);
    return 0;
}

void arm_reset(void) {
    arm_x = ARM_HOME_X;
    arm_y = ARM_HOME_Y;
    arm_z = ARM_HOME_Z;

    float angles[3];
    if (arm_ik_solve(arm_x, arm_y, arm_z, angles) == 0) {
        servo_write(&servo_base,     angles[0]);
        servo_write(&servo_shoulder, angles[1]);
        servo_write(&servo_elbow,    angles[2]);
    }
    ESP_LOGI(ARM_TAG, "Arm reset to home (%.2f, %.2f, %.2f)", arm_x, arm_y, arm_z);
}

void arm_get_position(float *x, float *y, float *z) {
    *x = arm_x;
    *y = arm_y;
    *z = arm_z;
}

void arm_init(void) {
    arm_pwm_timer_init();

    servo_init_channel(&servo_base);
    servo_init_channel(&servo_shoulder);
    servo_init_channel(&servo_elbow);

    arm_reset();
    ESP_LOGI(ARM_TAG, "Arm initialized at home (%.2f, %.2f, %.2f)", arm_x, arm_y, arm_z);
}