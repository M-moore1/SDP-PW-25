#ifndef ARM_H
#define ARM_H

#include "driver/ledc.h"
#include "driver/gpio.h"
#include <math.h>
 
// Arm kinematics constants
#define ARM_D1  2.5f    // Base vertical displacement (cm)
#define ARM_A2  6.25f   // Upper arm link length (cm)
#define ARM_A3  8.5f    // Forearm link length (cm)
 
// Default home position (cm)
#define ARM_HOME_X  -5.0f
#define ARM_HOME_Y   7.5f
#define ARM_HOME_Z   5.5f
 
// Joint angle limits (degrees)
#define ARM_THETA0_MIN  -50.0f
#define ARM_THETA0_MAX   50.0f
#define ARM_THETA1_MIN    0.0f
#define ARM_THETA1_MAX   90.0f
#define ARM_THETA2_MIN    0.0f
#define ARM_THETA2_MAX   90.0f
 
// Speed-to-step mapping
#define ARM_SPEED_MIN_STEP  0.1f   // cm per cmd at speed=1
#define ARM_SPEED_MAX_STEP  2.0f   // cm per cmd at speed=127
 
// Servo descriptor 
 
typedef struct {
    int            servo_pin;
    float          link_length;
    int            pwm_offset_us;
    float          current_angle;
    float          pwm;
    ledc_channel_t channel;
} servo_t;
 

void arm_pwm_timer_init(void);
void arm_init(void);
 
// Move arm to absolute (x, y, z) in cm.
// 0 on success, 1 if position is out of reach / joint limits exceeded.
int arm_move_to(float x, float y, float z);
 
// Move arm to home position and reset internal state.
void arm_reset(void);
 

void arm_get_position(float *x, float *y, float *z);
 
// Solve inverse kinematics for a target (x, y, z).
// Writes three joint angles (degrees) into servo_angles[3].
// 0 on success, 1 if position unreachable or outside joint limits.
int arm_ik_solve(float x, float y, float z, float servo_angles[3]);

#endif
