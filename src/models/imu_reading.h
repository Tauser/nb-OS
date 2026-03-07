#pragma once

struct ImuReading {
  float accelXg = 0.0f;
  float accelYg = 0.0f;
  float accelZg = 1.0f;
  bool valid = false;
};
