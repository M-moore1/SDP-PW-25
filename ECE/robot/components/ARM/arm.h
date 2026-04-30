#ifndef ARM_H
#define ARM_H

#include "driver/ledc.h"
#include "driver/gpio.h"
#include "pinout.h"
#include <math.h>

// Kinematics constants 
#define ARM_D1  2.5f
#define ARM_A2  6.0f
#define ARM_A3  8.5f

#define ARM_HOME_X   0.0f
#define ARM_HOME_Y   7.5f
#define ARM_HOME_Z   5.5f

#define ARM_THETA0_MIN   10.0f
#define ARM_THETA0_MAX   170.0f

#define ARM_THETA1_MIN    10.0f
#define ARM_THETA1_MAX   170.0f


#define ARM_THETA2_MIN    10.0f
#define ARM_THETA2_MAX   170.0f

#define ARM_SPEED_MIN_STEP  0.05f
#define ARM_SPEED_MAX_STEP  0.2f

/*
Each BLE command moves the arm 0.1–0.5in in one axis
That translates to maybe 2–10° of joint angle change
A typical servo does 60° in ~150ms, so 10° takes ~25ms
Your commands arrive every 50ms
*/

// Servo descriptor

typedef struct {
    int            servo_pin;
    float          link_length;
    int            pwm_offset_us;
    float          current_angle;   // angle currently written to hardware
    float          target_angle;    // mirrors current_angle — reserved for future interpolation
    float          pwm;
    ledc_channel_t channel;
} servo_t;



void arm_init(void);
int arm_move_to(float x, float y, float z);
void arm_reset(void);
void arm_get_position(float *x, float *y, float *z);
int arm_ik_solve(float x, float y, float z, float servo_angles[3]);

#endif
