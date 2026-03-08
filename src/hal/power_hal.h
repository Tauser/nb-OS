#pragma once

#include "../drivers/power/power_driver.h"

class PowerHAL {
public:
  explicit PowerHAL(PowerDriver& driver);

  bool init();
  bool sample(PowerRawReading& outReading) const;

private:
  PowerDriver& driver_;
};
