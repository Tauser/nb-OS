#pragma once

#include "imu_reading.h"
#include "touch_reading.h"
#include <stdint.h>

struct SensorSnapshot {
  ImuReading imu;
  TouchReading touch;
  uint32_t timestampMs = 0;
};
