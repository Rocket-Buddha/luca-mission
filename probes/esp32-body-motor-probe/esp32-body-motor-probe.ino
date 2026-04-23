#include <Arduino.h>

constexpr uint8_t SHCP_PIN = 18;
constexpr uint8_t EN_PIN = 16;
constexpr uint8_t DATA_PIN = 5;
constexpr uint8_t STCP_PIN = 17;
constexpr uint8_t PWM_A_PIN = 19;
constexpr uint8_t PWM_B_PIN = 23;

constexpr uint8_t M1_Forward = 128;
constexpr uint8_t M1_Backward = 64;
constexpr uint8_t M2_Forward = 32;
constexpr uint8_t M2_Backward = 16;
constexpr uint8_t M3_Forward = 2;
constexpr uint8_t M3_Backward = 4;
constexpr uint8_t M4_Forward = 1;
constexpr uint8_t M4_Backward = 8;

void applyDrive(uint8_t dirMask, int pwmA, int pwmB) {
  digitalWrite(EN_PIN, LOW);
  analogWrite(PWM_A_PIN, constrain(pwmA, 0, 255));
  analogWrite(PWM_B_PIN, constrain(pwmB, 0, 255));

  digitalWrite(STCP_PIN, LOW);
  shiftOut(DATA_PIN, SHCP_PIN, MSBFIRST, dirMask);
  digitalWrite(STCP_PIN, HIGH);

  Serial.printf("[PROBE] dir=0x%02X pwm19=%d pwm23=%d\n", dirMask, pwmA, pwmB);
}

void stopAll(unsigned long ms) {
  applyDrive(0x00, 0, 0);
  delay(ms);
}

void testPhase(const char *name, uint8_t dirMask, int pwmA, int pwmB, unsigned long ms) {
  Serial.printf("[PROBE] %s\n", name);
  applyDrive(dirMask, pwmA, pwmB);
  delay(ms);
  stopAll(800);
}

void setup() {
  Serial.begin(115200);

  pinMode(SHCP_PIN, OUTPUT);
  pinMode(EN_PIN, OUTPUT);
  pinMode(DATA_PIN, OUTPUT);
  pinMode(STCP_PIN, OUTPUT);
  pinMode(PWM_A_PIN, OUTPUT);
  pinMode(PWM_B_PIN, OUTPUT);

  digitalWrite(EN_PIN, LOW);
  digitalWrite(SHCP_PIN, LOW);
  digitalWrite(STCP_PIN, LOW);
  digitalWrite(DATA_PIN, LOW);

  Serial.println("[PROBE] motor shield probe start");
  stopAll(1500);
}

void loop() {
  testPhase("M1 forward", M1_Forward, 255, 0, 1800);
  testPhase("M1 backward", M1_Backward, 255, 0, 1800);

  testPhase("M2 forward", M2_Forward, 255, 0, 1800);
  testPhase("M2 backward", M2_Backward, 255, 0, 1800);

  testPhase("M3 forward", M3_Forward, 0, 255, 1800);
  testPhase("M3 backward", M3_Backward, 0, 255, 1800);

  testPhase("M4 forward", M4_Forward, 0, 255, 1800);
  testPhase("M4 backward", M4_Backward, 0, 255, 1800);

  testPhase("all forward", M1_Forward | M2_Forward | M3_Forward | M4_Forward, 255, 255, 2000);
  testPhase("all backward", M1_Backward | M2_Backward | M3_Backward | M4_Backward, 255, 255, 2000);

  Serial.println("[PROBE] cycle done");
  delay(3000);
}
