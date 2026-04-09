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


#include "driver/ledc.h"
#include <stdio.h>
#include "driver/ledc.h"
#include "esp_err.h"
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define Robot_arm_pwm_pin1 16
#define Robot_arm_pwm_pin2 17
#define Robot_arm_pwm_pin3 18
// Initializes Link length & displacement for the robot. In inches.
#define link_len1 2.5
#define link_len2 6.25
#define link_len3 8.5

void wait_ms(uint32_t ms) {
    // pdMS_TO_TICKS converts your milliseconds into system "ticks"
    vTaskDelay(pdMS_TO_TICKS(ms));
}

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
float acos22(float value, int* error_flag) {
  if (value < -1 || 1 < value) {(*error_flag)++; return 0;} // increments error flag if out of bound
  return ((acos(value))/3.14159)*180;}

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
    self->pwm = servo_angle * 100.0/9.0 + self->pwm_offset_us; 

    // 1. Set the duty in the hardware registers
    ledc_set_duty(LEDC_LOW_SPEED_MODE, self->channel, (self->pwm * 16384.0 /20000)); 
    // 16384.0 /20000 == 14 bit-resolution & s to microsec to duty%
    
    // 2. Tell the hardware to actually apply the change
    ledc_update_duty(LEDC_LOW_SPEED_MODE, self->channel);
}

int position_to_angles(float x, float y, float z, float* servo_angles){
    int error_flag = 0;

    float theta[2];
    theta[0] = atan22(y,x); // (returns "45" to "135" in code. In practice, -45 to 45)

    //rotates/displaces the xz-plane for easier 2d calculation
    x = cos22(theta[0])*x + sin22(theta[0])*y;
    // (returns "45" to "135" in code. In practice, -45 to 45)
    z = z-link_len1;

    theta[1] = atan22(z, x) + acos22( (x*x+z*z + link_len2*link_len2 - link_len3*link_len3) / (2 * sqrt(x*x+z*z) * link_len2), &error_flag);
    // phi + law of cosines formula on translated xz-plane

    theta[2] = 180 - acos22( (link_len3*link_len3 + link_len2*link_len2 - (x*x + z*z)) / (2 * link_len2 * link_len3), &error_flag);
    //compliment angle  + law of cosines formula

    // checks for angle 0,1,2 valid angle ranges.
    if (! (-50 < theta[0] && theta[0] < 50) )  error_flag++; 
    if (! (0 < theta[1] && theta[1] > 90) )    error_flag++;
    if (! (0 < theta[2] && theta[2] > 90) )    error_flag++;
    if(error_flag){ return 1; }

    // saves angles to theta list
    for (int i=0; i<3; i++){ 
        servo_angles[i] = theta[i];
    }
    return 0;
}

// Valid angles for angle1: 45,135
// Valid angles for angle 2: 45, 125
// Valid angles for angle 3: -120, 45  OR something diffreent? CHECK THIS ASAP.

void servo_initial_movement(servo_class* servo_1_addy, servo_class* servo_2_addy, servo_class* servo_3_addy){ 
  //Start at (0, 7.5, 5.5) AKA angles = (90, 92.2, 139.72)
  wait_ms(500);
  write_to_servo(servo_1_addy, 90);
  wait_ms(100);
  write_to_servo(servo_2_addy, 92.2);
  wait_ms(50);
  write_to_servo(servo_3_addy, 139.72); 
  wait_ms(50);
} // SAVE PREVIOUS POSITION TO EEPROM. THAT BECOMES NEW STARTUP POSITION
//Otherwise use this startup position


void app_main(void)
{

    float xb = 6.5; 
    float yb = 7.5; 
    float zb = 5.5; 
    float xk; float yk; float zk;
    // Initializes coordinate variables to store points in 3d

    pwm_timer_init();
    wait_ms(200);
    servo_class servo_1 = {Robot_arm_pwm_pin1, link_len1, 550, 45, 1000, LEDC_CHANNEL_4}; 
    servo_class servo_2 = {Robot_arm_pwm_pin2, link_len2, 500, 45, 1000, LEDC_CHANNEL_5};
    servo_class servo_3 = {Robot_arm_pwm_pin3, link_len3, 1120, 45, 1120, LEDC_CHANNEL_6};
    servo_init(&servo_1);
    servo_init(&servo_2);
    servo_init(&servo_3);
    servo_initial_movement(&servo_1, &servo_2, &servo_3);
    
    for(int i=0; i<2; i++){
        float servo_angles[3];
        if(position_to_angles(xb,yb,zb, servo_angles)){}// printf("Invalid coordinates\n");
        write_to_servo(&servo_1, servo_angles[0]);
        wait_ms(100);
        write_to_servo(&servo_2, servo_angles[1]);
        wait_ms(50);
        write_to_servo(&servo_3, servo_angles[2]);

        wait_ms(3000);
        if(position_to_angles(0.0,yb,zb, servo_angles)){}// printf("Invalid coordinates\n");
        write_to_servo(&servo_1, servo_angles[0]);
        write_to_servo(&servo_2, servo_angles[1]);
        write_to_servo(&servo_3, servo_angles[2]);

        wait_ms(3000);
        if(position_to_angles((-6.5),yb,zb, servo_angles)){}// printf("Invalid coordinates\n");
        write_to_servo(&servo_1, servo_angles[0]);
        write_to_servo(&servo_2, servo_angles[1]);
        write_to_servo(&servo_3, servo_angles[2]);

        wait_ms(3000);
        if(position_to_angles(0,yb,zb, servo_angles)){}// printf("Invalid coordinates\n");
        write_to_servo(&servo_1, servo_angles[0]);
        write_to_servo(&servo_2, servo_angles[1]);
        write_to_servo(&servo_3, servo_angles[2]);
        wait_ms(3000);
    }


    servo_initial_movement(&servo_1, &servo_2, &servo_3);

    
}












/*


Easy as heck python code. Use Python Shell

#Include Cooridnates here.
character = '''
'''

print(len(character))

new_string= character.replace(",","-")
newer_string = new_string.replace("\n","n")

print(newer_string)


*/




//Need Linear function with edge case if change in x is 0. Divide by 0 hehe.



//Need Brush off vs on page Mechanic is the last coordinate is within 0.1 inches of it's previous coordinate.


/*

//Function calculates the joint angle values provided the xyz coordinates.  (Also prints theta 1,2,3 in Serial Monitor)
float theta123(float xc, float yc, float zc) {
    

    printf("(");
    printf(xc);
    printf(",");
    printf(yc);
    printf(",");
    printf(zc);
    printfln(")");
    theta1 = atan22(yc,xc);

    //rotates/displaces the xz-plane for easier 2d calculation
    xc = cos22(theta1)*xc + sin22(theta1)*yc;
    zc = zc-link_len1;

    theta2 = atan22(zc, xc) + acos22( (xc*xc+zc*zc + link_len2*link_len2 - link_len3*link_len3) / (2 * sqrt(xc*xc+zc*zc) * link_len2) );
    // phi + law of cosines formula on translated xz-plane

    theta3 = 180 - acos22( (link_len3*link_len3 + link_len2*link_len2 - (xc*xc + zc*zc)) / (2 * link_len2 * link_len3) );
    //compliment angle  + law of cosines formula

    //Converts to PWM signal
    process_theta(1,theta1);
    process_theta(2,theta2);
    process_theta(3,theta3);


    //Performs move action
    angles123(thetapwm1, thetapwm2, thetapwm3);

    //Can delete afterwards
    printfln();
    printfln(theta1);
    printfln(theta2);
    printfln(theta3);
}


float Linear_Function(float xcurrent, float zcurrent, float xfinal, float zfinal){
  
  
  float zvalue = 0.000;
  float xvalue = 0.000;

  int lengthincrement = sqrt(pow((xfinal-xcurrent),2)+pow((zfinal-zcurrent),2))*10;
  
  float buffer= abs(xcurrent-xfinal);

  //Moving based on z axis
  if(buffer <= 0.5){
    float increment = (zcurrent-zfinal)/lengthincrement;
    float i = zcurrent;

    //Moving up
    if(zcurrent < zfinal){
      while (i<zfinal){
        zvalue = i;
        xvalue = ((xfinal-xcurrent)/(zfinal-zcurrent)) * (zvalue-zcurrent) + xcurrent;

        theta123(xvalue,y,zvalue);
        i -= increment;
      }}

    //Moving Down
    if(zcurrent > zfinal){
      while (i>zfinal){
        zvalue = i;
        xvalue = ((xfinal-xcurrent)/(zfinal-zcurrent)) * (zvalue-zcurrent) + xcurrent;

        theta123(xvalue,y,zvalue);
        i -= increment;
    }
    
}
}

//Moving based on x axis
  if(buffer > 0.5){

    float increment = (xcurrent-xfinal)/lengthincrement;
    float i = xcurrent;

    //Moving to the right
    if(xcurrent < xfinal){
      while (i<xfinal){
        xvalue = i;
        zvalue = ((zfinal-zcurrent)/(xfinal-xcurrent)) * (xvalue-xcurrent) + zcurrent;

        theta123(xvalue,y,zvalue);

        i -= increment;
      }}
    //Moving to the right
    if(xcurrent > xfinal){
      while (i > xfinal){
        xvalue = i;
        zvalue = ((zfinal-zcurrent)/(xfinal-xcurrent)) * (xvalue-xcurrent) + zcurrent;

        theta123(xvalue,y,zvalue);

        i -= increment;
      
      }}}
      delay(200);
}















int s;
String hold = "";
float xtarget,ztarget;
String n = "n";
String m = "-";

//Input coordinates here::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//Input coordinates here::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//Input coordinates here::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//Input coordinates here::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//Input coordinates here::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//Input coordinates here::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//Input coordinates here::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//Input coordinates here::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//Input coordinates here::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//Input coordinates here::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
String coordinates="190-426n190-426n177-386n186-340n227-309n272-298n316-288n349-262n383-242n422-226n455-216n491-209n519-209n552-211n586-213n614-219n634-227n667-249n688-268n721-280n757-290n773-321n777-347n783-373n780-406n759-419n737-416n731-393n718-370n688-357n667-360n640-372n629-396n629-419n639-445n659-463n681-462n714-454n727-436n737-419n608-421n586-421n557-422n531-422n501-422n472-421n436-419n403-419n360-419n337-386n309-352n265-360n244-388n232-416n242-442n259-459n286-465n309-462n332-445n342-419n193-422n355-295n649-285n593-239n504-229n411-247n357-293n";



//new proposed one
char Input_Append(int i){

  //Converts from string to character to string again
  char in = coordinates.charAt(i);
  String input = String(in);

  //Checks for a comma then saves holding value to xtarget, before clearing the holding value 
  if(input == m){
    xtarget = hold.toFloat();
    hold = "";}

  //Checks for a space then saves holding value to ztarget, before clearing the holding value and moving to the x,z coordinate.
  if(input == n){
    ztarget = hold.toFloat();
    hold = "";
    //Processing
    xtarget = ((xtarget-500)/100);
    ztarget = (1300-ztarget) / 100;



    //states current target
    printf(xtarget);
    printf(", ");
    printfln(ztarget);

    //calls a linear for loop or raises/lowers brush
    Linear_Function(xi,zi,xtarget,ztarget);

    //Saves current posiiton
    xi = xtarget;
    zi = ztarget;
  }
  //Checks for characters and tack on the input
  if(input != m && input != n){
    hold.concat(input);}
}




// Linear_Function(xi,zi,xtarget,ztarget); is the only one that matters
// remember to update linear function.
// this is depth like crazy.



void setup(){
    myservo1.attach(3);
    myservo2.attach(9);
    myservo3.attach(6);
    Serial.begin(9600); 

    //Sets link parameters
    link_len1 = 2.5; //base displacement
    link_len2 = 6.25; //5.8625
    link_len3 = 8.5; //long link
    //Test location, should be at top right of the page.
   
    xi = -5; 
    y = 7.5;
    zi = 5.5;

    delay(0);
}

//Create line function, make edge cases, take input from serial monitor,etc.
void loop(){
  s = coordinates.length();
  printfln(s);
  delay(2000);
  if(s!=0){
    for(int i = (s-1); i > 0; i--){
      Input_Append(s-i);
    }
  //resets the holding character
  hold = "";
  }
  delay(1000000000000);
}







//Adjusts theta/angle values and converts them to PWM signals. Each PWM input is different because each servo is oriented slightly differently.
float process_theta(int theta_number, float theta_value) {
    // Modify the global variables of theta, taking the theta number & value as arguments
    
    if (theta_number==1){   //θ_1 ∈ (45,135) == PWM(1000,2000), from -yx plane
      thetapwm1 = ((theta_value)*(100/9)+550);} 
      
    if (theta_number==2){   //θ_2 ∈ (45°,135°) == PWM(1000,1500), from xz plane
      thetapwm2 = ((theta_value)*(100/9)+500);} 

    if (theta_number==3){   //θ_3 ∈ (-120°, 45°) == PWM(2433, 1000), from xz plane
      thetapwm3 = ((theta_value)*(100/9)+1120);} 
}
// 20000 us is the max signal. so 1000 is 5% duty cycle. 
// 1000-2000us is the range for 0-180deg
// each servo has an offset. Respect their offset for this robot.






servo_class servo_1, servo_2, servo_3;
// Moves all 3 servos to each angle
float angles123(float th1, float th2, float th3) {


  myservo1.write(th1, 3, true);
  myservo2.write(th2, 3, true);
  myservo3.write(th3, 3, true);
  delay(0);
  //increase delay for larger increments between commands
}





void app_main() {}

*/
