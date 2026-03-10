#pragma once

struct PowerStatusPayload {
  int batteryPercent = 0;
  bool charging = false;
  int mode = 0;
  int batteryMillivolts = 0;
};
