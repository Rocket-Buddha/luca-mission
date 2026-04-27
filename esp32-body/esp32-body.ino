
#include "esp_camera.h"
#include "ACB_SmartCar_V2.h"
#include "ultrasonic.h"
#include <ESP32Servo.h>
#include <Arduino.h>
#include <stdarg.h>

#define Shoot_PIN 32  //shoot---200ms
#define FIXED_SERVO_PIN 25
#define TURN_SERVO_PIN 26

#define LED_Module1 2
#define LED_Module2 12
#define Left_sensor 35
#define Middle_sensor 36
#define Right_sensor 39
#define Buzzer 33

#define CMD_RUN 1
#define CMD_GET 2
#define CMD_STANDBY 3
#define CMD_TRACK_LEGACY 4
#define CMD_TRACK 5
#define CMD_AVOID 6
#define CMD_FOLLOW 7

//app music
#define C3 131
#define D3 147
#define E3 165
#define F3 175
#define G3 196
#define A3 221
#define B3 248

#define C4 262
#define D4 294
#define E4 330
#define F4 350
#define G4 393
#define A4 441
#define B4 495

#define C5 525
#define D5 589
#define E5 661
#define F5 700
#define G5 786
#define A5 882
#define B5 990
#define E6 1319
#define G6 1568
#define A6 1760
#define AS6 1865
#define B6 1976
#define C7 2093
#define D7 2349
#define E7 2637
#define F7 2794
#define G7 3136
#define A7 3520
#define N 0
#define HORN_NOTE G4


ACB_SmartCar_V2 Acebott;        //car
ultrasonic Ultrasonic;  //Ultrasonic
Servo fixedServo;       //Non-adjustable steering gear
Servo turnServo;       //Adjustable steering gear


int Left_Tra_Value;
int Middle_Tra_Value;
int Right_Tra_Value;
const int kTrackLeftOnThreshold = 185;
const int kTrackLeftOffThreshold = 176;
const int kTrackMiddleOnThreshold = 198;
const int kTrackMiddleOffThreshold = 190;
const int kTrackRightOnThreshold = 1020;
const int kTrackRightOffThreshold = 1000;
const uint8_t kTrackSensorSampleCount = 4;
int speeds = 250;
int leftDistance = 0;
int middleDistance = 0;
int rightDistance = 0;

String sendBuff;
String Version = "Firmware Version is 0.12.21";
byte dataLen, index_a = 0;
char buffer[52];
unsigned char prevc = 0;
bool isStart = false;
bool ED_client = true;
bool WA_en = false;
byte RX_package[17] = { 0 };
uint16_t angle = 90;
byte action = Stop, device;
byte val = 0;
char model_var = 0;
int UT_distance = 0;

int marioMelody[] = {
  E7, E7, 0, E7,
  0, C7, E7, 0,
  G7, 0, 0, 0,
  G6, 0, 0, 0,

  C7, 0, 0, G6,
  0, 0, E6, 0,
  0, A6, 0, B6,
  0, AS6, A6, 0,

  G6, E7, G7,
  A7, 0, F7, G7,
  0, E7, 0, C7,
  D7, B6, 0, 0,

  C7, 0, 0, G6,
  0, 0, E6, 0,
  0, A6, 0, B6,
  0, AS6, A6, 0,

  G6, E7, G7,
  A7, 0, F7, G7,
  0, E7, 0, C7,
  D7, B6, 0, 0
};

int marioTempo[] = {
  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,

  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,

  9, 9, 9,
  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,

  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,

  9, 9, 9,
  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,
};

const int marioLength = sizeof(marioMelody) / sizeof(marioMelody[0]);
bool hornActive = false;
bool melodyLoopActive = false;
bool melodyStepPlaying = false;
int melodyIndex = 0;
unsigned long melodyStepStartedAt = 0;
unsigned long melodyStepDurationMs = 0;

const int BATTERY_ADC_PIN = 34;
const int BATTERY_ADC_SAMPLE_COUNT = 16;
const unsigned long BATTERY_SAMPLE_INTERVAL_MS = 30000;
const float BATTERY_DIVIDER_R1_OHMS = 10000.0f;
const float BATTERY_DIVIDER_R2_OHMS = 4500.0f;
const float BATTERY_PACK_EMPTY_V = 6.0f;
const float BATTERY_PACK_FULL_V = 8.4f;

float batteryPackVoltage = 0.0f;
int batteryPercent = 0;
unsigned long lastBatterySampleMs = 0;

void logPacket(const char *tag, int length) {
  Serial.print(tag);
  Serial.print(" len=");
  Serial.print(length);
  Serial.print(" data=");
  for (int i = 0; i < length; i++) {
    if (i > 0) {
      Serial.print(' ');
    }
    if ((uint8_t)buffer[i] < 0x10) {
      Serial.print('0');
    }
    Serial.print((uint8_t)buffer[i], HEX);
  }
  Serial.println();
}
unsigned char readBuffer(int index_r) {
  return buffer[index_r];
}
void writeBuffer(int index_w, unsigned char c) {
  buffer[index_w] = c;
}

const char *driveCommandName(byte command) {
  switch (command) {
    case 0x00:
      return "stop";
    case 0x01:
      return "forward";
    case 0x02:
      return "backward";
    case 0x03:
      return "left";
    case 0x04:
      return "right";
    case 0x05:
      return "left-up";
    case 0x06:
      return "left-down";
    case 0x07:
      return "right-up";
    case 0x08:
      return "right-down";
    case 0x09:
      return "anticlockwise";
    case 0x0A:
      return "clockwise";
    default:
      return "unknown";
  }
}

void telemetryPrintf(const char *tag, const char *format, ...) {
  char message[160];
  va_list args;
  va_start(args, format);
  vsnprintf(message, sizeof(message), format, args);
  va_end(args);
  Serial.printf("[BODY][%s] %s\n", tag, message);
}

float clampFloat(float value, float minValue, float maxValue) {
  if (value < minValue) {
    return minValue;
  }
  if (value > maxValue) {
    return maxValue;
  }
  return value;
}

int calculateBatteryPercent(float packVoltage) {
  const float normalized =
    (packVoltage - BATTERY_PACK_EMPTY_V) / (BATTERY_PACK_FULL_V - BATTERY_PACK_EMPTY_V);
  return static_cast<int>(roundf(clampFloat(normalized, 0.0f, 1.0f) * 100.0f));
}

void updateBatteryTelemetry() {
  const unsigned long now = millis();
  if (lastBatterySampleMs != 0 && now - lastBatterySampleMs < BATTERY_SAMPLE_INTERVAL_MS) {
    return;
  }

  lastBatterySampleMs = now;

  // Warm up the ADC channel once before averaging the calibrated reads.
  analogReadMilliVolts(BATTERY_ADC_PIN);

  uint32_t totalMilliVolts = 0;
  for (int sampleIndex = 0; sampleIndex < BATTERY_ADC_SAMPLE_COUNT; sampleIndex++) {
    totalMilliVolts += analogReadMilliVolts(BATTERY_ADC_PIN);
  }

  const float senseVoltage =
    (static_cast<float>(totalMilliVolts) / BATTERY_ADC_SAMPLE_COUNT) / 1000.0f;
  const float dividerScale =
    (BATTERY_DIVIDER_R1_OHMS + BATTERY_DIVIDER_R2_OHMS) / BATTERY_DIVIDER_R2_OHMS;

  batteryPackVoltage = senseVoltage * dividerScale;
  batteryPercent = calculateBatteryPercent(batteryPackVoltage);

  telemetryPrintf("BATTERY", "pct=%d pack=%.2fV sense=%.3fV", batteryPercent, batteryPackVoltage, senseVoltage);
}

enum FUNCTION_MODE {
  STANDBY,
  FOLLOW,
  TRACK,
  AVOID,
} function_mode = STANDBY;

const char *functionModeName(int mode) {
  switch (mode) {
    case STANDBY:
      return "STANDBY";
    case FOLLOW:
      return "FOLLOW";
    case TRACK:
      return "TRACK";
    case AVOID:
      return "AVOID";
    default:
      return "UNKNOWN";
  }
}

void setFunctionMode(int mode, const char *reason) {
  FUNCTION_MODE previousMode = function_mode;
  function_mode = static_cast<FUNCTION_MODE>(mode);

  // Always reset motion when changing or rearming an autonomous mode.
  Acebott.Move(Stop, 0);
  fixedServo.write(90);

  Serial.printf(
    "[BODY] mode %s -> %s (%s)\n",
    functionModeName(previousMode),
    functionModeName(function_mode),
    reason
  );
}

void stopAutonomousMode(const char *reason) {
  if (function_mode != STANDBY) {
    setFunctionMode(STANDBY, reason);
  }
}

void setup() {
  Serial.setTimeout(10);  // Set the serial port timeout to 10 milliseconds
  Serial.begin(115200);   // Initialize serial communication, baud rate is 115200

  Acebott.Init();     // Initialize Acebott
  Ultrasonic.Init(13,14);  // Initialize the ultrasound module

  pinMode(LED_Module1, OUTPUT);   // Set pin 1 of LED module as output
  pinMode(LED_Module2, OUTPUT);   // Set pin 2 of the LED module as output
  pinMode(Shoot_PIN, OUTPUT);     // Set shooting pin as output
  pinMode(Buzzer, OUTPUT);        // Set buzzer pin as output
  pinMode(BATTERY_ADC_PIN, INPUT);  // Set battery sense pin as ADC input
  pinMode(Left_sensor, INPUT);    // Set the infrared left line pin as input
  pinMode(Middle_sensor, INPUT);  // Set the infrared middle line pin as input
  pinMode(Right_sensor, INPUT);   // Set the infrared right line pin as input

  analogReadResolution(12);
  analogSetPinAttenuation(BATTERY_ADC_PIN, ADC_11db);

  ESP32PWM::allocateTimer(1);          // Assign timer 1 to ESP32PWM library
  fixedServo.attach(FIXED_SERVO_PIN);           // Connect the servo to the FIXED_SERVO_PIN pin
  fixedServo.write(angle);                 // Set the servo angle to angle
  turnServo.attach(TURN_SERVO_PIN);  // Connect the servo to the FIXED_SERVO_PIN pin
  turnServo.write(angle);             // Set the servo angle to angle
  Acebott.Move(Stop, 0);               // Stop Acebott Movement
  delay(3000);                         // Delay 3 seconds

  Serial.println("start");
}

void loop() {
  RXpack_func();
  updateMelodyLoop();
  functionMode();
  updateBatteryTelemetry();
}

void functionMode() {
  switch (function_mode) {
    case FOLLOW:
      {
        model3_func();  // Enter the follow mode and call the model3_func() function
      }
      break;
    case TRACK:
      {
        trackMode3Sensor();  // Enter the 3-sensor tracking mode.
      }
      break;
    case AVOID:
      {
        model2_func();  // Enter obstacle avoidance mode and call the model2_func() function
      }
      break;
    default:
      break;
  }
}

void Receive_data()  // Receive data
{
}

int readTrackSensorAverage(uint8_t pin) {
  uint32_t total = 0;
  for (uint8_t sampleIndex = 0; sampleIndex < kTrackSensorSampleCount; sampleIndex++) {
    total += analogRead(pin);
  }
  return static_cast<int>(total / kTrackSensorSampleCount);
}

bool trackSensorOnLine(int value, bool latchedOnLine, int onThreshold, int offThreshold) {
  if (latchedOnLine) {
    return value >= offThreshold;
  }
  return value >= onThreshold;
}

void model2_func()  // OA
{
  static unsigned long lastTelemetryMs = 0;
  const char *actionLabel = "forward";

  fixedServo.write(90);
  UT_distance = Ultrasonic.Ranging();
  //Serial.print("UT_distance:  ");
  //Serial.println(UT_distance);
  middleDistance = UT_distance;

  if (middleDistance <= 25) {
    Acebott.Move(Stop, 0);
    delay(100);
    int randNumber = random(1, 4);
    switch (randNumber) {
      case 1:
        actionLabel = "backward-left";
        Acebott.Move(Backward, 180);
        delay(500);
        Acebott.Move(Move_Left, 180);
        delay(500);
        break;
      case 2:
        actionLabel = "rotate-right";
        Acebott.Move(Clockwise, 180);
        delay(1000);
        break;
      case 3:
        actionLabel = "rotate-left";
        Acebott.Move(Contrarotate, 180);
        delay(1000);
        break;
      case 4:
        actionLabel = "backward-right";
        Acebott.Move(Backward, 180);
        delay(500);
        Acebott.Move(Move_Right, 180);
        delay(500);
        break;
    }
  } else {
    actionLabel = "forward";
    Acebott.Move(Forward, 180);
  }

  if (millis() - lastTelemetryMs >= 350) {
    lastTelemetryMs = millis();
    telemetryPrintf("AVOID", "distance=%d action=%s", middleDistance, actionLabel);
  }
}

void model3_func()  // follow model
{
  static unsigned long lastTelemetryMs = 0;
  const char *actionLabel = "stop";

  fixedServo.write(90);
  UT_distance = Ultrasonic.Ranging();
  //Serial.println(UT_distance);
  if (UT_distance < 15) {
    actionLabel = "backward";
    Acebott.Move(Backward, 200);
  } else if (15 <= UT_distance && UT_distance <= 20) {
    actionLabel = "stop";
    Acebott.Move(Stop, 0);
  } else if (20 <= UT_distance && UT_distance <= 25) {
    actionLabel = "forward-trim";
    Acebott.Move(Forward, speeds - 70);
  } else if (25 <= UT_distance && UT_distance <= 50) {
    actionLabel = "forward-fast";
    Acebott.Move(Forward, 220);
  } else {
    actionLabel = "search-stop";
    Acebott.Move(Stop, 0);
  }

  if (millis() - lastTelemetryMs >= 300) {
    lastTelemetryMs = millis();
    telemetryPrintf("FOLLOW", "distance=%d action=%s", UT_distance, actionLabel);
  }
}

void trackMode3Sensor()
{
  static unsigned long lastTelemetryMs = 0;
  static unsigned long lastLineSeenMs = 0;
  static bool leftLatchedOnLine = false;
  static bool middleLatchedOnLine = false;
  static bool rightLatchedOnLine = false;
  static int lastLineDirection = 0;
  static bool correctionActive = false;
  static bool correctionTurning = false;
  static int correctionDirection = 0;
  static unsigned long correctionPhaseStartedMs = 0;
  static unsigned long correctionTurnDurationMs = 0;
  const unsigned long lineRecoveryWindowMs = 260;
  const unsigned long correctionBrakeMs = 70;
  const unsigned long softCorrectionTurnMs = 90;
  const unsigned long hardCorrectionTurnMs = 130;
  const int straightLeftSpeed = 180;
  const int straightRightSpeed = 180;
  const int softCorrectionSpinSpeed = 190;
  const int hardCorrectionSpinSpeed = 200;
  const int recoverySpinSpeed = 170;
  const int recoveryStraightSpeed = 150;
  const char *actionLabel = "straight";
  const unsigned long now = millis();

  fixedServo.write(90);
  Left_Tra_Value = readTrackSensorAverage(Left_sensor);
  Middle_Tra_Value = readTrackSensorAverage(Middle_sensor);
  Right_Tra_Value = readTrackSensorAverage(Right_sensor);

  bool leftOnLine = trackSensorOnLine(
    Left_Tra_Value,
    leftLatchedOnLine,
    kTrackLeftOnThreshold,
    kTrackLeftOffThreshold
  );
  bool middleOnLine = trackSensorOnLine(
    Middle_Tra_Value,
    middleLatchedOnLine,
    kTrackMiddleOnThreshold,
    kTrackMiddleOffThreshold
  );
  bool rightOnLine = trackSensorOnLine(
    Right_Tra_Value,
    rightLatchedOnLine,
    kTrackRightOnThreshold,
    kTrackRightOffThreshold
  );

  leftLatchedOnLine = leftOnLine;
  middleLatchedOnLine = middleOnLine;
  rightLatchedOnLine = rightOnLine;

  const uint8_t linePattern =
    (leftOnLine ? 0b100 : 0) |
    (middleOnLine ? 0b010 : 0) |
    (rightOnLine ? 0b001 : 0);

  const bool centeredPattern = linePattern == 0b010 || linePattern == 0b101 || linePattern == 0b111;
  const bool leftCorrectionPattern = linePattern == 0b100 || linePattern == 0b110;
  const bool rightCorrectionPattern = linePattern == 0b001 || linePattern == 0b011;
  const bool hardCorrectionPattern = linePattern == 0b100 || linePattern == 0b001;

  if (centeredPattern) {
    correctionActive = false;
    correctionTurning = false;
    correctionDirection = 0;
    correctionTurnDurationMs = 0;
    lastLineSeenMs = now;
    lastLineDirection = 0;
    actionLabel = "straight";
    Acebott.DriveSides(straightLeftSpeed, straightRightSpeed);
  } else if (leftCorrectionPattern || rightCorrectionPattern) {
    const int requestedDirection = leftCorrectionPattern ? -1 : 1;
    const unsigned long requestedTurnDurationMs = hardCorrectionPattern ? hardCorrectionTurnMs : softCorrectionTurnMs;

    lastLineSeenMs = now;
    lastLineDirection = requestedDirection;

    if (!correctionActive ||
        correctionDirection != requestedDirection ||
        correctionTurnDurationMs != requestedTurnDurationMs) {
      correctionActive = true;
      correctionTurning = false;
      correctionDirection = requestedDirection;
      correctionPhaseStartedMs = now;
      correctionTurnDurationMs = requestedTurnDurationMs;
    }

    if (!correctionTurning && now - correctionPhaseStartedMs < correctionBrakeMs) {
      actionLabel = correctionDirection < 0 ? "brake-left" : "brake-right";
      Acebott.Move(Stop, 0);
    } else {
      if (!correctionTurning) {
        correctionTurning = true;
        correctionPhaseStartedMs = now;
      }

      if (now - correctionPhaseStartedMs < correctionTurnDurationMs) {
        if (correctionDirection < 0) {
          actionLabel = hardCorrectionPattern ? "correct-hard-left" : "correct-soft-left";
          Acebott.Move(
            Contrarotate,
            hardCorrectionPattern ? hardCorrectionSpinSpeed : softCorrectionSpinSpeed
          );
        } else {
          actionLabel = hardCorrectionPattern ? "correct-hard-right" : "correct-soft-right";
          Acebott.Move(
            Clockwise,
            hardCorrectionPattern ? hardCorrectionSpinSpeed : softCorrectionSpinSpeed
          );
        }
      } else {
        correctionActive = false;
        correctionTurning = false;
        actionLabel = "straight";
        Acebott.DriveSides(straightLeftSpeed, straightRightSpeed);
      }
    }
  } else if (linePattern == 0b000) {
    correctionActive = false;
    correctionTurning = false;
    if (lastLineSeenMs != 0 && now - lastLineSeenMs <= lineRecoveryWindowMs) {
      if (lastLineDirection < 0) {
        actionLabel = "recover-left";
        Acebott.Move(Contrarotate, recoverySpinSpeed);
      } else if (lastLineDirection > 0) {
        actionLabel = "recover-right";
        Acebott.Move(Clockwise, recoverySpinSpeed);
      } else {
        actionLabel = "recover-straight";
        Acebott.DriveSides(recoveryStraightSpeed, recoveryStraightSpeed);
      }
    } else {
      // After a short recovery window, stop instead of running blind.
      actionLabel = "line-lost-stop";
      Acebott.Move(Stop, 0);
    }
  } else {
    correctionActive = false;
    correctionTurning = false;
    lastLineSeenMs = now;
    lastLineDirection = 0;
    actionLabel = "straight";
    Acebott.DriveSides(straightLeftSpeed, straightRightSpeed);
  }

  if (millis() - lastTelemetryMs >= 250) {
    lastTelemetryMs = millis();
    telemetryPrintf(
      "TRACK",
      "L=%d M=%d R=%d line=%d%d%d action=%s",
      Left_Tra_Value,
      Middle_Tra_Value,
      Right_Tra_Value,
      leftOnLine ? 1 : 0,
      middleOnLine ? 1 : 0,
      rightOnLine ? 1 : 0,
      actionLabel
    );
  }
}

void Servo_Move(int angles)  //servo
{
  turnServo.write(map(angles, 1, 180, 130, 70));
  delay(10);
}

void Horn_on() {
  hornActive = true;
  tone(Buzzer, HORN_NOTE);
}
void Horn_off() {
  hornActive = false;
  if (!melodyLoopActive) {
    noTone(Buzzer);
    return;
  }

  melodyStepPlaying = false;
  melodyStepDurationMs = 0;
  melodyStepStartedAt = 0;
  noTone(Buzzer);
}

void Melody_on() {
  melodyLoopActive = true;
  melodyIndex = 0;
  melodyStepPlaying = false;
  melodyStepDurationMs = 0;
  melodyStepStartedAt = 0;
  if (!hornActive) {
    noTone(Buzzer);
  }
}

void Melody_off() {
  melodyLoopActive = false;
  melodyStepPlaying = false;
  melodyStepDurationMs = 0;
  melodyStepStartedAt = 0;
  if (!hornActive) {
    noTone(Buzzer);
  }
}

void updateMelodyLoop() {
  if (!melodyLoopActive || hornActive) {
    return;
  }

  unsigned long now = millis();
  if (melodyStepPlaying && now - melodyStepStartedAt < melodyStepDurationMs) {
    return;
  }

  int note = marioMelody[melodyIndex];
  int tempoUnit = marioTempo[melodyIndex];
  unsigned long noteDuration = 1000UL / tempoUnit;

  melodyStepStartedAt = now;
  melodyStepDurationMs = noteDuration + noteDuration / 3;
  melodyStepPlaying = true;

  if (note == 0) {
    noTone(Buzzer);
  } else {
    tone(Buzzer, note);
  }

  melodyIndex++;
  if (melodyIndex >= marioLength) {
    melodyIndex = 0;
  }
}
void Buzzer_run(int M) {
  switch (M) {
    case 0x05:
      Horn_on();
      break;
    case 0x06:
      Melody_on();
      break;
    case 0x07:
      Melody_off();
      break;
    case 0x00:
      Horn_off();
      break;
    default:
      break;
  }
}

void runModule(int device) {
  val = readBuffer(12);
  Serial.printf("[BODY] runModule device=0x%02X val=0x%02X speeds=%d\n", device, val, speeds);
  switch (device) {
    case 0x0C:
      {
        switch (val) {
          case 0x01:
            Acebott.Move(Forward, speeds);
            break;
          case 0x02:
            Acebott.Move(Backward, speeds);
            break;
          case 0x03:
            Acebott.Move(Move_Left, speeds);
            break;
          case 0x04:
            Acebott.Move(Move_Right, speeds);
            break;
          case 0x05:
            Acebott.Move(Top_Left, speeds);
            break;
          case 0x06:
            Acebott.Move(Bottom_Left, speeds);
            break;
          case 0x07:
            Acebott.Move(Top_Right, speeds);
            break;
          case 0x08:
            Acebott.Move(Bottom_Right, speeds);
            break;
          case 0x0A:
            Acebott.Move(Clockwise, speeds);
            break;
          case 0x09:
            Acebott.Move(Contrarotate, speeds);
            break;
          case 0x00:
            Acebott.Move(Stop, 0);
            break;
          default:
            break;
        }
        telemetryPrintf("MANUAL", "direction=%s speed=%d", driveCommandName(val), val == 0x00 ? 0 : speeds);
      }
      break;
    case 0x02:
      {
        Servo_Move(val);
        telemetryPrintf("SERVO", "angle=%d", val);
      }
      break;
    case 0x03:
      {
        Buzzer_run(val);
        if (val == 0x05) {
          telemetryPrintf("AUDIO", "horn=on");
        } else if (val == 0x06) {
          telemetryPrintf("AUDIO", "melody=on");
        } else if (val == 0x07) {
          telemetryPrintf("AUDIO", "melody=off");
        } else if (val == 0x00) {
          telemetryPrintf("AUDIO", "horn=off");
        } else {
          telemetryPrintf("AUDIO", "unknown=%d", val);
        }
      }
      break;
    case 0x05:
      {
        digitalWrite(LED_Module1, val);
        digitalWrite(LED_Module2, val);
        telemetryPrintf("LED", "state=%d", val);
      }
      break;
    case 0x08:
      {
        digitalWrite(Shoot_PIN, HIGH);
        delay(200);
        digitalWrite(Shoot_PIN, LOW);
        telemetryPrintf("SHOOT", "triggered");
      }
      break;
    case 0x0D:
      {
        speeds = val;
        telemetryPrintf("SPEED", "value=%d", speeds);
      }
      break;
  }
}
void parseData() {
  isStart = false;
  int action = readBuffer(9);
  int device = readBuffer(10);
  int index2 = readBuffer(2);
  int packetLen = index2 + 1;

  logPacket("[BODY] packet", packetLen);
  Serial.printf(
    "[BODY] parse action=0x%02X device=0x%02X val=0x%02X index2=%d mode=%s\n",
    action,
    device,
    readBuffer(12),
    index2,
    functionModeName(function_mode)
  );

  switch (action) {
    case CMD_RUN:
      Serial.println("[BODY] CMD_RUN");
      if (device == 0x0C) {
        stopAutonomousMode("manual drive command");
      }
      runModule(device);
      break;
    case CMD_STANDBY:
      Serial.println("[BODY] CMD_STANDBY");
      setFunctionMode(STANDBY, "standby command");
      break;
    case CMD_TRACK_LEGACY:
      Serial.println("[BODY] CMD_TRACK_LEGACY");
      setFunctionMode(TRACK, "legacy track command");
      break;
    case CMD_TRACK:
      Serial.println("[BODY] CMD_TRACK");
      setFunctionMode(TRACK, "track command");
      break;
    case CMD_AVOID:
      Serial.println("[BODY] CMD_AVOID");
      setFunctionMode(AVOID, "avoidance command");
      break;
    case CMD_FOLLOW:
      Serial.println("[BODY] CMD_FOLLOW");
      setFunctionMode(FOLLOW, "follow command");
      break;
    default:
      Serial.printf("[BODY] unknown action=0x%02X\n", action);
      break;
  }
}

void RXpack_func()  //Receive data
{
  if (Serial.available() > 0) {
    unsigned char c = Serial.read() & 0xff;
    static unsigned int rxCount = 0;
    rxCount++;
    if (rxCount <= 32) {
      Serial.printf("[BODY] rx 0x%02X\n", c);
    }
    //Serial.write(c);
    if (c == 0x55 && isStart == false) {
      if (prevc == 0xff) {
        index_a = 1;
        isStart = true;
      }
    } else {
      prevc = c;
      if (isStart) {
        if (index_a == 2) {
          dataLen = c;
        } else if (index_a > 2) {
          dataLen--;
        }
        writeBuffer(index_a, c);
      }
    }
    index_a++;
    if (index_a > 120) {
      index_a = 0;
      isStart = false;
    }
    if (isStart && dataLen == 0 && index_a > 3) {
      isStart = false;
      parseData();
      index_a = 0;
    }
    // functionMode();
  }
}
