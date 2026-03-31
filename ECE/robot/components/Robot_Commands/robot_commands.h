#ifndef ROBOT_COMMAND_H
#define ROBOT_COMMAND_H

#include "../stepper_motor/stepper_motor.h"
#include "pinout.h"
#include <stdio.h>
#include <string.h>

//Robot State Variables
extern volatile int security_flag;
extern volatile uint16_t AC;        // Stored in NVS
extern volatile int motor_power;
extern char robot_name[32];         // Stored in NVS
extern volatile int arm_power;
extern volatile int sys_shtdwn;


void send_ack(uint16_t id, uint8_t result, int secure, uint64_t instr_specfic);
void control_cmd(control_format_t ctrl, step_mot_t* F_L, step_mot_t* F_R, step_mot_t* B_L, step_mot_t* B_R);
void arm_cmd   (arm_format_t arm, step_mot_t* F_L, step_mot_t* F_R, step_mot_t* B_L, step_mot_t* B_R);
void system_cmd (system_format_t sys, step_mot_t* F_L, step_mot_t* F_R, step_mot_t* B_L, step_mot_t* B_R);
void query_cmd  (query_format_t query, step_mot_t* F_L, step_mot_t* F_R, step_mot_t* B_L, step_mot_t* B_R);

void send_imu(int part);
void send_health_report();
void send_HPA();

#endif