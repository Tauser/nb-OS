#pragma once

#include "../models/power_state.h"

class IPower {
public:
  virtual ~IPower() = default;

  virtual void init() = 0;
  virtual void update(unsigned long nowMs) = 0;

  virtual int getBatteryPercent() const = 0;
  virtual bool isCharging() const = 0;
  virtual PowerMode getPowerMode() const = 0;
  virtual const PowerState& getPowerState() const = 0;
};
