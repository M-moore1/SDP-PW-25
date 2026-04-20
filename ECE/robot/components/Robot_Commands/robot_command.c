#include "robot_commands.h"
#include "Robot_BLE.h"
#include "imu.h"
#include "arm.h"
#include "aes_gcm_encrypt.h"

volatile int security_flag = 0;
volatile uint16_t AC = 0x3FF;  // PUT IN NVS
volatile int motor_power = 1;
char robot_name[32] = DEVICE_NAME; // PUT IN NVS
volatile int arm_power = 1;
volatile int sys_shtdwn = 0;

void send_ack(uint16_t id, uint8_t result, int secure, uint64_t instr_specfic) {
    robot_bt_packet_t response = {0};

    response.ack.pl = 1;
    response.ack.type = ACK_CMD;  
    response.ack.id = id;             
    response.ack.result_code = result;
    response.ack.instruction_specific = instr_specfic;
    send_cmd(response.bytes, secure);

    ESP_LOGI(CMD_TAG, "Sent ACK for ID %d with Result %d", id, result);
}

void control_cmd(control_format_t ctrl, step_mot_t* F_L, step_mot_t* F_R, step_mot_t* B_L, step_mot_t* B_R){
    if (motor_power == 0){
        ESP_LOGI(CMD_TAG, "Control CMD - Motor OFF");
        send_ack(ctrl.id, RESULT_CMD_FAILURE, security_flag, MOTORS_DISABLED);
        return;
    }

    bool w = ctrl.w; bool a = ctrl.a;
    bool s = ctrl.s; bool d = ctrl.d;
    uint8_t speed = ctrl.speed;

    // Check if any valid move input exists
    bool any_input = w || s || a || d;
    if (!any_input) return;

    // Enable all motors on any valid move — caller handles disable
    stepper_enable(F_L); 
    stepper_enable(F_R);
    stepper_enable(B_L);
    stepper_enable(B_R);

    ESP_LOGI(CMD_TAG, "Motor Moving");
    /*
      FRONT OF THE BOT
    F_L               F_R
    
    
    B_L               B_R

    */
    if (w && !s && !a && !d) {
        ESP_LOGI(CMD_TAG, "Forward at speed %d", speed);
        motor_pulse(F_L, speed, 1);
        motor_pulse(F_R, speed, 0);
        motor_pulse(B_L, speed, 1); 
        motor_pulse(B_R, speed, 0);
    }
    else if (s && !w && !a && !d) {
        ESP_LOGI(CMD_TAG, "Backward at speed %d", speed);
        motor_pulse(F_L, speed, 0);
        motor_pulse(F_R, speed, 1);
        motor_pulse(B_L, speed, 0);
        motor_pulse(B_R, speed, 1);
    }
    else if (d && !w && !s && !a) {
        ESP_LOGI(CMD_TAG, "In Place Right at speed %d", speed);
        motor_pulse(F_L, speed, 1);
        motor_pulse(F_R, speed, 1);
        motor_pulse(B_L, speed, 1); 
        motor_pulse(B_R, speed, 1);  
    }
    else if (a && !w && !s && !d) {
        ESP_LOGI(CMD_TAG, "In Place Left at speed %d", speed);
        motor_pulse(F_L, speed, 0);
        motor_pulse(F_R, speed, 0);
        motor_pulse(B_L, speed, 0); 
        motor_pulse(B_R, speed, 0);
    }
    else if (w && d) {
        ESP_LOGI(CMD_TAG, "Diagonal FWD-Right at speed %d", speed);
        motor_pulse(F_L, speed, 1);
        motor_pulse(F_R, speed/2, 0);
        motor_pulse(B_L, speed, 1); 
        motor_pulse(B_R, speed/2, 0);  
    }
    else if (w && a) {
        ESP_LOGI(CMD_TAG, "Diagonal FWD-Left at speed %d", speed);
        motor_pulse(F_L, speed/2, 1);
        motor_pulse(F_R, speed, 0);
        motor_pulse(B_L, speed/2, 1); 
        motor_pulse(B_R, speed, 0); 
    }
    else if (s && d) {
        ESP_LOGI(CMD_TAG, "Diagonal BWD-Right at speed %d", speed);
        motor_pulse(F_L, speed, 0);
        motor_pulse(F_R, speed/2, 1);
        motor_pulse(B_L, speed, 0);
        motor_pulse(B_R, speed/2, 1);
    }
    else if (s && a) {
        ESP_LOGI(CMD_TAG, "Diagonal BWD-Left at speed %d", speed);
        motor_pulse(F_L, speed/2, 0);
        motor_pulse(F_R, speed, 1);
        motor_pulse(B_L, speed/2, 0);
        motor_pulse(B_R, speed, 1);
    }

    send_ack(ctrl.id, RESULT_SUCCESS, security_flag, NO_INFO);
}

void arm_cmd(arm_format_t arm, step_mot_t* F_L, step_mot_t* F_R, step_mot_t* B_L, step_mot_t* B_R){
    ESP_LOGI(CMD_TAG, "Executing Arm CMD");
    if (arm.reset) {
        arm_reset();
        ESP_LOGI(CMD_TAG, "Arm reset to home");
        send_ack(arm.id, RESULT_SUCCESS, security_flag, NO_INFO);
        return;
    }

    // Map speed (1–127) to step size in cm
    float step = ARM_SPEED_MIN_STEP + 
                 (arm.speed / 127.0f) * (ARM_SPEED_MAX_STEP - ARM_SPEED_MIN_STEP);

    // Read current position and apply deltas
    float x, y, z;
    arm_get_position(&x, &y, &z);

    // Z axis — unchanged
    if (arm.up)    z += step;
    if (arm.down)  z -= step;

    // X axis — left/right buttons now control reach (in/out)
    if (arm.right)  x += step;   // left  → extend in  (was: arm.in)
    if (arm.left) x -= step;   // right → retract out (was: arm.out)

    // Y axis — in/out buttons now control yaw (turn left/right)
    if (arm.in)    y += step;   // in  → turn right (was: arm.right)
    if (arm.out)   y -= step;   // out → turn left  (was: arm.left)

    // arm_move_to solves IK internally and rejects bad positions
    if (arm_move_to(x, y, z) != 0) {
        ESP_LOGW(CMD_TAG, "ARM move rejected (%.2f, %.2f, %.2f)", x, y, z);
        send_ack(arm.id, RESULT_CMD_FAILURE, security_flag, ARM_CORDINATES_ISSUE);
        return;
    }
    send_ack(arm.id, RESULT_SUCCESS, security_flag, NO_INFO);
}

void system_cmd(system_format_t sys, step_mot_t* F_L, step_mot_t* F_R, step_mot_t* B_L, step_mot_t* B_R){
    ESP_LOGI(CMD_TAG, "Executing System CMD");
 
    uint64_t inst_type = (uint64_t)sys.instruction;
    uint16_t authorization_code = (uint16_t)sys.ac;
    uint32_t payload = (uint32_t)sys.specific;
    

    if (AC != authorization_code) { 
        ESP_LOGW(CMD_TAG, "System CMD - Incorrect Authorization Code");
        send_ack(sys.id, RESULT_AUTH_FAIL, security_flag, NO_INFO );
        return;
    }

    uint8_t result = RESULT_SUCCESS;
    uint64_t instr_spc_rsp = NO_INFO ;

    switch (inst_type)
    {
        
        case SECURITY_LEVEL:
        if (payload != 0 && payload != 1) {
            ESP_LOGW(CMD_TAG, "System CMD - Security LVL Not Possible");
            result = RESULT_INVALID_PARAMS;
            break;
        }

        ESP_LOGI(CMD_TAG, "System CMD - Security Flag Updated %d", payload);
        security_flag = payload;

        // Re-apply BLE security params to match new level
        ble_apply_security_params();

        result = RESULT_SUCCESS;
        instr_spc_rsp = security_flag ? SECURITY_ON : SECURITY_OFF;
        break;
        
        case ROBOT_POWER:
            if( payload != 0 && payload != 1){
                ESP_LOGW(CMD_TAG, "System CMD - Motor Setting Unclear");
                result = RESULT_INVALID_PARAMS;
                break;
            }
            motor_power = payload;
            result = RESULT_SUCCESS;
            if(motor_power){instr_spc_rsp = MOTORS_ENABLED; }
            else          {instr_spc_rsp = MOTORS_DISABLED;}

        break;
        
        case ROBOT_NAME_CHANGE:

            char *new_name_ptr = (char *)payload;
            if (new_name_ptr != NULL){
                strncpy(robot_name, new_name_ptr, sizeof(robot_name) - 1);
                robot_name[sizeof(robot_name) - 1] = '\0';

                esp_ble_gap_set_device_name(robot_name);
        
                ESP_LOGI(CMD_TAG, "Robot renamed to: %s", robot_name);
                result = RESULT_SUCCESS;
                instr_spc_rsp = NAME_UPDATED;

            }else {
                ESP_LOGW(CMD_TAG, "System CMD - Name changed failed");
                result = RESULT_INVALID_PARAMS;
                instr_spc_rsp = NAME_CHANGE_FAILED;
            }
        break;

        case UPDATE_AUTH_CODE:
            const uint32_t MAX_AC_VALUE = 0x3FF;

            if (payload > MAX_AC_VALUE) {
                ESP_LOGW(CMD_TAG, "System CMD - Auth Code Update Failed");
                
                result = RESULT_INVALID_PARAMS;
                instr_spc_rsp = AUTH_CODE_UPDATE_FAIL;
                break; 
            }
            uint16_t masked_value = (uint16_t)(payload & 0x3FF);
            AC = masked_value;
            ESP_LOGI(CMD_TAG, "Auth Code updated");
            result = RESULT_SUCCESS;
            instr_spc_rsp = AUTH_CODE_UPDATED;
        break;
        
        case ARM_POWER_CMD:
            if( payload != 0 && payload != 1){
                ESP_LOGW(CMD_TAG, "System CMD - Arm Power Unclear");
                result = RESULT_INVALID_PARAMS;
                break;
            }

            arm_power = payload;
            result = RESULT_SUCCESS;
            if(arm_power){instr_spc_rsp = ARM_ENABLED; }
            else         {instr_spc_rsp = ARM_DISABLED;}

        break;

        case EMERGENCY_SHTDWN:
            result = RESULT_CMD_FAILURE;
        break;

        default:
            result = RESULT_UNSUPPORTED_CMD;
            break;
    }

    send_ack(sys.id, result, security_flag, instr_spc_rsp);
}
void query_cmd(query_format_t query, step_mot_t* F_L, step_mot_t* F_R, step_mot_t* B_L, step_mot_t* B_R){
    ESP_LOGI(CMD_TAG, "Executing Query CMD");
    
    uint64_t inst_type = (uint64_t)query.instruction;
    uint8_t result = RESULT_SUCCESS;
    uint64_t instr_spc_rsp = NO_INFO;

    switch (inst_type)
    {
        case SECURITY_STATUS: 
            if(security_flag){instr_spc_rsp = SECURITY_ON; }
            else             {instr_spc_rsp = SECURITY_OFF;}

        break;

        case MOTOR_STATUS:
            if(motor_power){instr_spc_rsp = MOTORS_ENABLED; }
            else           {instr_spc_rsp = MOTORS_DISABLED;}
        break;

        case CURRENT_POSITION:
            // TODO: READ IMU
            result = RESULT_CMD_FAILURE;
        break;

        case ROBOT_BATT:
            // TODO : Battery Check
            result = RESULT_CMD_FAILURE;
        break;

        case ARM_POWER:
            if(arm_power){instr_spc_rsp = ARM_ENABLED; }
            else         {instr_spc_rsp = ARM_DISABLED;}
        break;

        case SHTDWN_STATUS:
            if(sys_shtdwn){instr_spc_rsp = SHTDWN_ENABLED; }
            else          {instr_spc_rsp = SHTDWN_DISABLED;}
        break;

        default:
            result = RESULT_UNSUPPORTED_CMD;
        break;

    }


    send_ack(query.id, result, security_flag, instr_spc_rsp);
}


void send_imu(int part) {
    robot_bt_packet_t pkt = {0};

    // PART 0: NAV
    if (part == 0) {
        pkt.nav.pl   = 1;
        pkt.nav.type = ROBOT_UPDATE_CMD;
        pkt.nav.part = 0; 

        pkt.nav.speed = 0; 

        // Position in mm
        pkt.nav.pos_x = 0;
        pkt.nav.pos_y = 0;
        pkt.nav.pos_z = 0;

        send_cmd(pkt.bytes, security_flag);
    }

    // PART 1: POSE
    else if (part == 1) {
        pkt.pose.pl   = 1;
        pkt.pose.type = ROBOT_UPDATE_CMD;
        pkt.pose.part = 1; 

        pkt.pose.yaw   = (uint32_t)(g_imu_euler.yaw * 1000.0f);
        pkt.pose.pitch = (int32_t)(g_imu_euler.pitch * 1000.0f);
        pkt.pose.roll  = (int32_t)(g_imu_euler.roll * 1000.0f);

        send_cmd(pkt.bytes, security_flag);
    }

    // PART 2: INERTIA
    else if (part == 2) {
        pkt.inert.pl   = 1;
        pkt.inert.type = ROBOT_UPDATE_CMD;
        pkt.inert.part = 2; 

        pkt.inert.accel_x = (int16_t)(g_imu_accel.x * 10.0f);
        pkt.inert.accel_y = (int16_t)(g_imu_accel.y * 10.0f);
        pkt.inert.accel_z = (int16_t)(g_imu_accel.z * 10.0f);

        pkt.inert.gyro_x  = (int16_t)(g_imu_gyro.x_degs * 10.0f);
        pkt.inert.gyro_y  = (int16_t)(g_imu_gyro.y_degs * 10.0f);
        pkt.inert.gyro_z  = (int16_t)(g_imu_gyro.z_degs * 10.0f);

        send_cmd(pkt.bytes, security_flag);
    }
    else {
        send_imu(0);
        vTaskDelay(pdMS_TO_TICKS(5));

        send_imu(1);
        vTaskDelay(pdMS_TO_TICKS(5));

        send_imu(2);
    }
}

void send_health_report(){
    robot_bt_packet_t health_report = {0};
    health_report.health.pl   = 1;
    health_report.health.type = HEALTH_CMD;
    health_report.health.battery  = 100; // TODO
    health_report.health.sec_en = security_flag;
    health_report.health.motor_en = motor_power;
    health_report.health.arm_en = arm_power;

    send_cmd(health_report.bytes, security_flag);

    ESP_LOGI(CMD_TAG, "Sending Health Report");
}

void send_HPA(){

}
