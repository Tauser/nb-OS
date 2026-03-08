#pragma once

#include <stdint.h>

enum class PowerMode : uint8_t {
  Normal = 0,
  LowPower,
  Charging,
  Sleep
};

struct PowerState {
  int batteryPercent = 100;
  int batteryMillivolts = 0;
  bool charging = false;
  PowerMode mode = PowerMode::Normal;

  bool valid() const {
    return batteryPercent >= 0 && batteryPercent <= 100;
  }
};
