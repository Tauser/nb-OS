#pragma once

struct PowerRawReading {
  int batteryMillivolts = 0;
  bool charging = false;
  bool valid = false;
};

class PowerDriver {
public:
  bool init();
  bool sample(PowerRawReading& outReading) const;
  bool isReady() const;

private:
  bool ready_ = false;
};
