#include "touch_driver.h"

#include "../../config/hardware_config.h"
#include "../../config/hardware_pins.h"
#include <Arduino.h>

bool TouchDriver::init() {
  pinMode(HardwarePins::Touch::DATA, INPUT);
  ready_ = true;
  return true;
}

bool TouchDriver::read(TouchReading& outReading) const {
  outReading = TouchReading{};

  if (!ready_) {
    return false;
  }

  const int raw = analogRead(HardwarePins::Touch::DATA);
  outReading.rawValue = raw;
  outReading.touched = raw >= HardwareConfig::Touch::ANALOG_TOUCH_THRESHOLD;
  outReading.valid = true;
  return true;
}

bool TouchDriver::isReady() const {
  return ready_;
}
