#include "ultrasonic.h"

namespace {
constexpr unsigned long kEchoTimeoutUs = 30000UL;
constexpr long kNoEchoDistanceCm = 500L;
}

ultrasonic::ultrasonic() : trig_pin_(0), echo_pin_(0) {}

void ultrasonic::Init(uint8_t trigPin, uint8_t echoPin) {
  trig_pin_ = trigPin;
  echo_pin_ = echoPin;

  pinMode(trig_pin_, OUTPUT);
  pinMode(echo_pin_, INPUT);
  digitalWrite(trig_pin_, LOW);
}

long ultrasonic::Ranging(uint8_t unit) {
  digitalWrite(trig_pin_, LOW);
  delayMicroseconds(2);
  digitalWrite(trig_pin_, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig_pin_, LOW);

  const unsigned long duration = pulseIn(echo_pin_, HIGH, kEchoTimeoutUs);
  if (duration == 0) {
    return unit == INC ? static_cast<long>(kNoEchoDistanceCm / 2.54) : kNoEchoDistanceCm;
  }

  const long distanceCm = static_cast<long>(duration / 58.0);
  if (unit == INC) {
    return static_cast<long>(distanceCm / 2.54);
  }
  return distanceCm;
}
