#include "ACB_SmartCar_V2.h"
#include "Arduino.h"

void ACB_SmartCar_V2::Init() {
    pinMode(SHCP_PIN, OUTPUT);
    pinMode(EN_PIN, OUTPUT);
    pinMode(DATA_PIN, OUTPUT);
    pinMode(STCP_PIN, OUTPUT);
    pinMode(PWM_A_PIN, OUTPUT);
    pinMode(PWM_B_PIN, OUTPUT);

    digitalWrite(EN_PIN, LOW);
    digitalWrite(STCP_PIN, LOW);
    digitalWrite(DATA_PIN, LOW);
    digitalWrite(SHCP_PIN, LOW);

    for (int i = 0; i < 4; ++i) {
        motorSpeeds_[i] = 0;
    }

    flushMotors();
}

void ACB_SmartCar_V2::motorControl(uint8_t motor, int speed) {
    if (motor < 1 || motor > 4) {
        return;
    }

    motorSpeeds_[motor - 1] = constrain(speed, -255, 255);
    flushMotors();
}

void ACB_SmartCar_V2::DriveSides(int leftSpeed, int rightSpeed) {
    motorSpeeds_[0] = constrain(leftSpeed, -255, 255);
    motorSpeeds_[1] = constrain(leftSpeed, -255, 255);
    motorSpeeds_[2] = constrain(rightSpeed, -255, 255);
    motorSpeeds_[3] = constrain(rightSpeed, -255, 255);
    flushMotors();
}

void ACB_SmartCar_V2::flushMotors() {
    uint8_t dir = 0;
    int pwmA = 0;
    int pwmB = 0;

    if (motorSpeeds_[0] > 0) {
        dir |= M1_Forward;
    } else if (motorSpeeds_[0] < 0) {
        dir |= M1_Backward;
    }
    pwmA = max(pwmA, abs(motorSpeeds_[0]));

    if (motorSpeeds_[1] > 0) {
        dir |= M2_Forward;
    } else if (motorSpeeds_[1] < 0) {
        dir |= M2_Backward;
    }
    pwmA = max(pwmA, abs(motorSpeeds_[1]));

    if (motorSpeeds_[2] > 0) {
        dir |= M3_Forward;
    } else if (motorSpeeds_[2] < 0) {
        dir |= M3_Backward;
    }
    pwmB = max(pwmB, abs(motorSpeeds_[2]));

    if (motorSpeeds_[3] > 0) {
        dir |= M4_Forward;
    } else if (motorSpeeds_[3] < 0) {
        dir |= M4_Backward;
    }
    pwmB = max(pwmB, abs(motorSpeeds_[3]));

    digitalWrite(EN_PIN, LOW);
    analogWrite(PWM_A_PIN, pwmA);
    analogWrite(PWM_B_PIN, pwmB);

    digitalWrite(STCP_PIN, LOW);
    shiftOut(DATA_PIN, SHCP_PIN, MSBFIRST, dir);
    digitalWrite(STCP_PIN, HIGH);

    Serial.printf(
        "[MOTOR] dir=0x%02X pwm19=%d pwm23=%d m1=%d m2=%d m3=%d m4=%d\n",
        dir,
        pwmA,
        pwmB,
        motorSpeeds_[0],
        motorSpeeds_[1],
        motorSpeeds_[2],
        motorSpeeds_[3]
    );
}

void ACB_SmartCar_V2::Move(int Dir, int Speed) {
    if (Dir == Forward) {
        motorControl(1, Speed);
        motorControl(2, Speed);
        motorControl(3, Speed);
        motorControl(4, Speed);
    }

    if (Dir == Backward) {
        motorControl(1, -Speed);
        motorControl(2, -Speed);
        motorControl(3, -Speed);
        motorControl(4, -Speed);
    }

    if (Dir == Move_Right) {
        motorControl(1, Speed);
        motorControl(2, -Speed);
        motorControl(3, -Speed);
        motorControl(4, Speed);
    }

    if (Dir == Move_Left) {
        motorControl(1, -Speed);
        motorControl(2, Speed);
        motorControl(3, Speed);
        motorControl(4, -Speed);
    }

    if (Dir == Clockwise) {
        motorControl(1, Speed);
        motorControl(2, Speed);
        motorControl(3, -Speed);
        motorControl(4, -Speed);
    }

    if (Dir == Contrarotate) {
        motorControl(1, -Speed);
        motorControl(2, -Speed);
        motorControl(3, Speed);
        motorControl(4, Speed);
    }

    if (Dir == Top_Left) {
        motorControl(1, 0);
        motorControl(2, Speed);
        motorControl(3, Speed);
        motorControl(4, 0);
    }

    if (Dir == Top_Right) {
        motorControl(1, Speed);
        motorControl(2, 0);
        motorControl(3, 0);
        motorControl(4, Speed);
    }

    if (Dir == Bottom_Right) {
        motorControl(1, 0);
        motorControl(2, -Speed);
        motorControl(3, -Speed);
        motorControl(4, 0);
    }

    if (Dir == Bottom_Left) {
        motorControl(1, -Speed);
        motorControl(2, 0);
        motorControl(3, 0);
        motorControl(4, -Speed);
    }

    if (Dir == Stop) {
        motorControl(1, 0);
        motorControl(2, 0);
        motorControl(3, 0);
        motorControl(4, 0);
    }
}
