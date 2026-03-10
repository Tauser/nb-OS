#pragma once

struct HealthStatusPayload {
  int healthScore = 0;
  int primaryAnomaly = 0;
  bool degraded = false;
  bool heartbeatOk = false;
  bool renderOk = false;
  bool memoryOk = false;
  bool eventLoadOk = false;
};
