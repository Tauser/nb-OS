#pragma once

#include <stdint.h>

struct VisionSnapshot {
  bool valid = false;
  bool realBackend = false;
  uint32_t frameId = 0;
  uint32_t timestampMs = 0;
  uint32_t captureUs = 0;
  uint16_t avgLuma = 0;     // 0..1023
  uint16_t motionScore = 0; // 0..1023
};
