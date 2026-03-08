#pragma once

#include <stdint.h>

enum class HealthAnomalyCode : uint8_t {
  None = 0,
  HeartbeatTimeout,
  LowRenderFps,
  LowMemory,
  HighEventLoad
};

struct HealthSnapshot {
  uint16_t renderFps = 0;
  uint16_t eventRatePerSecond = 0;
  uint32_t freeHeapBytes = 0;
  uint32_t minFreeHeapBytes = 0;

  bool heartbeatOk = true;
  bool renderOk = true;
  bool memoryOk = true;
  bool eventLoadOk = true;

  uint8_t healthScore = 100;
  bool degraded = false;
  HealthAnomalyCode primaryAnomaly = HealthAnomalyCode::None;
};
