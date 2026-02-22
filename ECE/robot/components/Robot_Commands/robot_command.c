#include "robot_commands.h"
#include "robot_bt.h"

volatile int security_flag = 0;
volatile uint16_t AC = 0x3FF;

void control_cmd(control_format_t ctrl, step_mot_t* F_L, step_mot_t* F_R, step_mot_t* B_L, step_mot_t* B_R){
    printf("\nExecuting a Control CMD\n");

    bool w = ctrl.w; bool a = ctrl.a; 
    bool s = ctrl.s; bool d = ctrl.d;
    uint8_t speed = ctrl.speed;

    if(w) { 
        printf("\nMotor moving forward at speed %d\n", speed);
        motor_pulse(F_L, speed, 0); 
    }
    if(s) { 
        printf("\nMotor moving backwards at speed %d\n", speed);
        motor_pulse(F_L, speed, 1); 
    }

}

void pose_cmd(uint64_t inst, step_mot_t* F_L, step_mot_t* F_R, step_mot_t* B_L, step_mot_t* B_R){
    printf("\nExecuting a Pose CMD\n");
}


void system_cmd(system_format_t sys, step_mot_t* F_L, step_mot_t* F_R, step_mot_t* B_L, step_mot_t* B_R){
    printf("\nExecuting a System CMD\n");

    system_instructions_t inst_type = (system_instructions_t)sys.instruction;
    uint16_t authorization_code = (uint16_t)sys.ac;

    uint32_t payload = (uint32_t)sys.specific;

    if (AC != authorization_code) { 
        printf("Unathorized System Change");
        return;
    }

    switch (inst_type)
    {
        case SECURITY_LEVEL:
            
            security_flag = payload;
            printf("\nNew Security Level: %d\n", security_flag );
            // RESPOND and SEND back in to the UI
            // esp_spp_write(spp_handle, sizeof(message)-1, (uint8_t *)message);
            break;
        
        default:
            break;
    }
}
void query_cmd(query_format_t query, step_mot_t* F_L, step_mot_t* F_R, step_mot_t* B_L, step_mot_t* B_R){
    printf("\nExecuting a Query CMD\n");
}