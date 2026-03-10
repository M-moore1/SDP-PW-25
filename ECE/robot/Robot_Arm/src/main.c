#include "driver/ledc.h"
#include <stdio.h>
#include "driver/ledc.h"
#include "esp_err.h"
#include <math.h>

void pwm_timer_init(){
    // 1. Configure the Timer
    ledc_timer_config_t arm_timer = {
            .speed_mode       = LEDC_LOW_SPEED_MODE, // ESP32 standard
            .duty_resolution  = LEDC_TIMER_14_BIT,   // 16,384 steps of precision
            .timer_num        = LEDC_TIMER_0,
            .freq_hz          = 50,                  // Standard servo frequency
            .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&arm_timer);
}

//Rewriting the trig functions to output/input degrees rather than radians
float sin22(float value) { return ((sin(value/180*3.14159))); }
float cos22(float value) { return ((cos(value/180*3.14159))); }
float atan22(float ycord, float xcord) { return ((atan2(ycord,xcord))/3.14159)*180; }
float acos22(float value) {return ((acos(value))/3.14159)*180;}

//base displacement //5.8625 //last link

// theta1, 2,3 corespond to base servo, 2nd servo, end effector servo.
typedef struct {
    int servo_pin;
    float link_length;
    int pwm_offset_us;
    float current_angle;
    float pwm;
    ledc_channel_t channel;
} servo_class;

void servo_init(servo_class* self){ // Configure a Channel
    ledc_channel_config_t ledc_config = {
        .gpio_num       = self->servo_pin,
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = self->channel,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = LEDC_TIMER_0,
        .duty           = 0, // Start at 0
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_config);
}

// convert to duty and to 14bit. Then convert to duty percent. 
// divide by 20000 for 50Hz. 20k us or 20ms. Then scale by 16384.

void write_to_servo(servo_class* self, float servo_angle){

    // 0. Process angle
    self->current_angle = servo_angle;
    self->pwm = servo_angle * 10.0/9.0 + self->pwm_offset_us; 

    // 1. Set the duty in the hardware registers
    ledc_set_duty(LEDC_LOW_SPEED_MODE, self->channel, (self->pwm * 16384.0 /20000)); 
    // 14 bit-res & us to duty%
    
    // 2. Tell the hardware to actually apply the change
    ledc_update_duty(LEDC_LOW_SPEED_MODE, self->channel);
}

// Initializes Link length & displacement for the robot.
float d1 = 2.5; 
float a2 = 6.25; 
float a3 = 8.5;

int position_to_angles(float x, float y, float z, float* servo_angles){
    int error_flag = 0;
    float theta[2];
    theta[0] = atan22(y,x);

    //rotates/displaces the xz-plane for easier 2d calculation
    x = cos22(theta[0])*x + sin22(theta[0])*y;
    z = z-d1;

    theta[1] = atan22(z, x) + acos22( (x*x+z*z + a2*a2 - a3*a3) / (2 * sqrt(x*x+z*z) * a2) );
    // phi + law of cosines formula on translated xz-plane

    theta[2] = 180 - acos22( (a3*a3 + a2*a2 - (x*x + z*z)) / (2 * a2 * a3) );
    //compliment angle  + law of cosines formula

    // checks for angle 0,1,2 valid angle ranges.
    if (! (-50 < theta[0] && theta[0] < 50) )  error_flag++; 
    if (! (0 < theta[1] && theta[1] > 90) )    error_flag++;
    if (! (0 < theta[2] && theta[2] > 90) )    error_flag++;
    if(error_flag){
        return 1;
    }

    // saves angles to theta list
    for (int i=0; i<3; i++){ 
        servo_angles[i] = theta[i];
    }
    return 0;
}

float xb = -5.0; 
float yb = 7.5; 
float zb = 5.5; 
float xk; float yk; float zk;
// Initializes coordinate variables to store points in 3d

void app_main(void)
{
    pwm_timer_init();
    servo_class servo_1 = {16, d1, 550, 45, 1000, LEDC_CHANNEL_4}; 
    servo_class servo_2 = {17, a2, 500, 45, 1000, LEDC_CHANNEL_5};
    servo_class servo_3 = {18, a3, 1120, 45, 1120, LEDC_CHANNEL_6};
    servo_init(&servo_1);
    servo_init(&servo_2);
    servo_init(&servo_3);

    float servo_angles[3];
    if(position_to_angles(xb,yb,zb, servo_angles)) printf("Invalid coordinates\n");
    write_to_servo(&servo_1, servo_angles[0]);
    write_to_servo(&servo_2, servo_angles[1]);
    write_to_servo(&servo_3, servo_angles[2]);
    
    
}
