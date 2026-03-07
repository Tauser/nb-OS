#pragma once

#include "../../models/touch_reading.h"

class TouchDriver {
public:
  bool init();
  bool read(TouchReading& outReading) const;
  bool isReady() const;

private:
  bool ready_ = false;
};
