#ifndef _ACB_SMARTCAR_V2_H__
#define _ACB_SMARTCAR_V2_H__

#include "Arduino.h"

#define SHCP_PIN 18
#define EN_PIN 16
#define DATA_PIN 5
#define STCP_PIN 17
#define PWM_A_PIN 19
#define PWM_B_PIN 23

const int Forward = 163;
const int Backward = 92;
const int Move_Left = 106;
const int Move_Right = 149;
const int Top_Left = 34;
const int Bottom_Left = 72;
const int Top_Right = 129;
const int Bottom_Right = 20;
const int Stop = 0;
const int Contrarotate = 83;
const int Clockwise = 172;
const int Moedl1 = 25;
const int Moedl2 = 26;
const int Moedl3 = 27;
const int Moedl4 = 28;
const int MotorLeft = 230;
const int MotorRight = 231;
const int M1_Forward = 128;
const int M1_Backward = 64;
const int M2_Forward = 32;
const int M2_Backward = 16;
const int M3_Forward = 2;
const int M3_Backward = 4;
const int M4_Forward = 1;
const int M4_Backward = 8;

class ACB_SmartCar_V2 {
public:
    void Init();
    void Move(int Dir, int Speed);
    void DriveSides(int leftSpeed, int rightSpeed);
    void motorControl(uint8_t motor, int speed);

private:
    int motorSpeeds_[4];

    void flushMotors();
};

#endif
