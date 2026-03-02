#ifndef ROBOT_COMMAND_H
#define ROBOT_COMMAND_H

#include "stepper_motor.h"
#include "pinout.h"
#include <stdio.h>
#include <string.h>

//Robot State Variables
extern volatile int security_flag;
extern volatile uint16_t AC;


void control_cmd(control_format_t ctrl, step_mot_t* F_L, step_mot_t* F_R, step_mot_t* B_L, step_mot_t* B_R);
void pose_cmd   (uint64_t inst, step_mot_t* F_L, step_mot_t* F_R, step_mot_t* B_L, step_mot_t* B_R);
void system_cmd (system_format_t sys, step_mot_t* F_L, step_mot_t* F_R, step_mot_t* B_L, step_mot_t* B_R);
void query_cmd  (query_format_t query, step_mot_t* F_L, step_mot_t* F_R, step_mot_t* B_L, step_mot_t* B_R);

#endif