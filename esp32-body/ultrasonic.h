#ifndef ACEBOTT_ULTRASONIC_H
#define ACEBOTT_ULTRASONIC_H

#include <Arduino.h>

#define CM 1
#define INC 0

class ultrasonic {
public:
  ultrasonic();
  void Init(uint8_t trigPin, uint8_t echoPin);
  long Ranging(uint8_t unit = CM);

private:
  uint8_t trig_pin_;
  uint8_t echo_pin_;
};

#endif
