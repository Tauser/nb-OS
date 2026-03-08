#include "power_hal.h"

PowerHAL::PowerHAL(PowerDriver& driver)
    : driver_(driver) {}

bool PowerHAL::init() {
  return driver_.init();
}

bool PowerHAL::sample(PowerRawReading& outReading) const {
  return driver_.sample(outReading);
}
