#include "ACB_SmartCar_V2.h"
#include "Arduino.h"

namespace {
constexpr uint8_t kMotorCount = 4;

bool motorStateEquals(const int (&lhs)[kMotorCount], const int (&rhs)[kMotorCount]) {
    for (uint8_t index = 0; index < kMotorCount; ++index) {
        if (lhs[index] != rhs[index]) {
            return false;
        }
    }
    return true;
}

void copyMotorState(int (&destination)[kMotorCount], const int (&source)[kMotorCount]) {
    for (uint8_t index = 0; index < kMotorCount; ++index) {
        destination[index] = source[index];
    }
}

bool motorStateIsIdle(const int (&state)[kMotorCount]) {
    for (uint8_t index = 0; index < kMotorCount; ++index) {
        if (state[index] != 0) {
            return false;
        }
    }
    return true;
}

void describeMotorState(const int (&state)[kMotorCount], char *buffer, size_t bufferSize) {
    const int frontLeft = state[0];
    const int rearLeft = state[1];
    const int frontRight = state[2];
    const int rearRight = state[3];

    if (motorStateIsIdle(state)) {
        snprintf(buffer, bufferSize, "stop");
        return;
    }

    if (frontLeft == rearLeft && frontRight == rearRight) {
        if (frontLeft == frontRight) {
            if (frontLeft > 0) {
                snprintf(buffer, bufferSize, "forward");
                return;
            }
            if (frontLeft < 0) {
                snprintf(buffer, bufferSize, "backward");
                return;
            }
        }

        if (frontLeft > 0 && frontRight > 0) {
            snprintf(buffer, bufferSize, "forward-trim(left=%d,right=%d)", frontLeft, frontRight);
            return;
        }

        if (frontLeft < 0 && frontRight < 0) {
            snprintf(buffer, bufferSize, "backward-trim(left=%d,right=%d)", abs(frontLeft), abs(frontRight));
            return;
        }

        if (frontLeft > 0 && frontRight < 0) {
            if (frontLeft == -frontRight) {
                snprintf(buffer, bufferSize, "rotate-clockwise");
            } else {
                snprintf(buffer, bufferSize, "rotate-clockwise(left=%d,right=%d)", frontLeft, abs(frontRight));
            }
            return;
        }

        if (frontLeft < 0 && frontRight > 0) {
            if (-frontLeft == frontRight) {
                snprintf(buffer, bufferSize, "rotate-counterclockwise");
            } else {
                snprintf(buffer, bufferSize, "rotate-counterclockwise(left=%d,right=%d)", abs(frontLeft), frontRight);
            }
            return;
        }
    }

    if (frontLeft < 0 &&
        rearLeft > 0 &&
        frontRight > 0 &&
        rearRight < 0 &&
        (-frontLeft == rearLeft) &&
        (rearLeft == frontRight) &&
        (frontRight == -rearRight)) {
        snprintf(buffer, bufferSize, "strafe-left");
        return;
    }

    if (frontLeft > 0 &&
        rearLeft < 0 &&
        frontRight < 0 &&
        rearRight > 0 &&
        (frontLeft == -rearLeft) &&
        (-rearLeft == -frontRight) &&
        (-frontRight == rearRight)) {
        snprintf(buffer, bufferSize, "strafe-right");
        return;
    }

    if (frontLeft == 0 && rearLeft > 0 && frontRight > 0 && rearRight == 0 && rearLeft == frontRight) {
        snprintf(buffer, bufferSize, "diag-forward-left");
        return;
    }

    if (frontLeft > 0 && rearLeft == 0 && frontRight == 0 && rearRight > 0 && frontLeft == rearRight) {
        snprintf(buffer, bufferSize, "diag-forward-right");
        return;
    }

    if (frontLeft < 0 && rearLeft == 0 && frontRight == 0 && rearRight < 0 && frontLeft == rearRight) {
        snprintf(buffer, bufferSize, "diag-backward-left");
        return;
    }

    if (frontLeft == 0 && rearLeft < 0 && frontRight < 0 && rearRight == 0 && rearLeft == frontRight) {
        snprintf(buffer, bufferSize, "diag-backward-right");
        return;
    }

    snprintf(
        buffer,
        bufferSize,
        "custom(front-left=%d,rear-left=%d,front-right=%d,rear-right=%d)",
        frontLeft,
        rearLeft,
        frontRight,
        rearRight
    );
}
}  // namespace

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
        lastLoggedMotorSpeeds_[i] = 0;
    }
    hasLastLoggedMotorState_ = false;

    flushMotors();
}

void ACB_SmartCar_V2::motorControl(uint8_t motor, int speed) {
    if (motor < 1 || motor > 4) {
        return;
    }

    motorSpeeds_[motor - 1] = constrain(speed, -255, 255);
    flushMotors();
}

void ACB_SmartCar_V2::setMotorSpeeds(int motor1Speed, int motor2Speed, int motor3Speed, int motor4Speed) {
    motorSpeeds_[0] = constrain(motor1Speed, -255, 255);
    motorSpeeds_[1] = constrain(motor2Speed, -255, 255);
    motorSpeeds_[2] = constrain(motor3Speed, -255, 255);
    motorSpeeds_[3] = constrain(motor4Speed, -255, 255);
    flushMotors();
}

void ACB_SmartCar_V2::DriveSides(int leftSpeed, int rightSpeed) {
    setMotorSpeeds(leftSpeed, leftSpeed, rightSpeed, rightSpeed);
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

    if (!hasLastLoggedMotorState_ || !motorStateEquals(motorSpeeds_, lastLoggedMotorSpeeds_)) {
        char previousState[96];
        char currentState[96];
        const bool previousWasIdle = !hasLastLoggedMotorState_ || motorStateIsIdle(lastLoggedMotorSpeeds_);
        const bool currentIsIdle = motorStateIsIdle(motorSpeeds_);

        describeMotorState(lastLoggedMotorSpeeds_, previousState, sizeof(previousState));
        describeMotorState(motorSpeeds_, currentState, sizeof(currentState));

        if (!currentIsIdle && previousWasIdle) {
            Serial.printf(
                "[MOTOR] start state=%s front-left=%d rear-left=%d front-right=%d rear-right=%d\n",
                currentState,
                motorSpeeds_[0],
                motorSpeeds_[1],
                motorSpeeds_[2],
                motorSpeeds_[3]
            );
        } else if (currentIsIdle && !previousWasIdle) {
            Serial.printf("[MOTOR] stop state=%s\n", previousState);
        } else if (!currentIsIdle) {
            Serial.printf(
                "[MOTOR] change from=%s to=%s front-left=%d rear-left=%d front-right=%d rear-right=%d\n",
                previousState,
                currentState,
                motorSpeeds_[0],
                motorSpeeds_[1],
                motorSpeeds_[2],
                motorSpeeds_[3]
            );
        }

        copyMotorState(lastLoggedMotorSpeeds_, motorSpeeds_);
        hasLastLoggedMotorState_ = true;
    }
}

void ACB_SmartCar_V2::Move(int Dir, int Speed) {
    switch (Dir) {
        case Forward:
            setMotorSpeeds(Speed, Speed, Speed, Speed);
            break;
        case Backward:
            setMotorSpeeds(-Speed, -Speed, -Speed, -Speed);
            break;
        case Move_Right:
            setMotorSpeeds(Speed, -Speed, -Speed, Speed);
            break;
        case Move_Left:
            setMotorSpeeds(-Speed, Speed, Speed, -Speed);
            break;
        case Clockwise:
            setMotorSpeeds(Speed, Speed, -Speed, -Speed);
            break;
        case Contrarotate:
            setMotorSpeeds(-Speed, -Speed, Speed, Speed);
            break;
        case Top_Left:
            setMotorSpeeds(0, Speed, Speed, 0);
            break;
        case Top_Right:
            setMotorSpeeds(Speed, 0, 0, Speed);
            break;
        case Bottom_Right:
            setMotorSpeeds(0, -Speed, -Speed, 0);
            break;
        case Bottom_Left:
            setMotorSpeeds(-Speed, 0, 0, -Speed);
            break;
        case Stop:
            setMotorSpeeds(0, 0, 0, 0);
            break;
        default:
            break;
    }
}
