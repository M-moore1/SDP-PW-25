#include "stepper_motor.h"
#include "esp_log.h"

// Stops the motor after 60 ms countdown is complete
static void motor_stop_callback(void* arg) {
    step_mot_t* motor = (step_mot_t*)arg;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, motor->channel, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, motor->channel);
}

//Initialize a instance of the stepper motor
void motor_init(step_mot_t* m, const int step_pin, const int dir_pin, const int en_pin, ledc_channel_t channel){
    m->dir_gpio = dir_pin;
    m->en_gpio = en_pin;
    m->step_gpio = step_pin;
    m->status = MOTOR_IDLE;
    m->channel = channel;

    gpio_set_direction(m->dir_gpio, GPIO_MODE_OUTPUT);
    gpio_set_direction(m->en_gpio, GPIO_MODE_OUTPUT);
    gpio_set_level(m->en_gpio, 1);

    m->timer_sel = (ledc_timer_t)(channel / 2); 

    // 1. Configure LEDC Timer
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = m->timer_sel, 
        .duty_resolution  = STEPPER_LEDC_RESOLUTION,
        .freq_hz          = 300000,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    // 2. Configure Individual Channel
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = m->channel,
        .timer_sel      = m->timer_sel,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = m->step_gpio,
        .duty           = 0,
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);

    // 3. Create high-precision ESP Timer
    esp_timer_create_args_t timer_args = {
        .callback = motor_stop_callback,
        .arg = m,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "motor_stop_timer"
    };
    
    esp_timer_create(&timer_args, &m->stop_timer);
}

void stepper_enable(step_mot_t* m){
    gpio_set_level(m->en_gpio, 1); 
    m->status = MOTOR_IDLE;
}

void stepper_disable(step_mot_t* m){
    gpio_set_level(m->en_gpio, 0); 
    m->status = MOTOR_DISABLE;
}

uint32_t map_speed_to_hz(int speed) {
    if (speed <= 0) return 0;
    if (speed == 1) return 200;
    if (speed >= 100) return 3000;

    return 200 + (speed - 1) * (3000 - 200) / (100 - 1);
}

void motor_pulse(step_mot_t *motor, uint32_t speed, int dir){
    // Get speed in hz and set motor direction
    uint32_t freq_hz = map_speed_to_hz(speed);
    gpio_set_level(motor->dir_gpio, dir);

    // if speed is set to 0 return with nothing
    if (freq_hz == 0) {
        esp_timer_stop(motor->stop_timer);
        motor->status = MOTOR_IDLE;
        ledc_set_duty(LEDC_LOW_SPEED_MODE, motor->channel, 0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, motor->channel);
        return;
    }

    esp_timer_stop(motor->stop_timer);
    

    ledc_set_freq(LEDC_LOW_SPEED_MODE, motor->timer_sel, freq_hz);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, motor->channel, STEPPER_LEDC_DUTY);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, motor->channel);
    
    // Start timer that goes for 60 ms
    motor->status = MOTOR_RUNNING;
    esp_timer_start_once(motor->stop_timer, PULSE_DURATION_US);
}
