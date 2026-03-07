#pragma once

class IPower {
public:
  virtual ~IPower() = default;
  virtual int getBatteryPercent() const = 0;
  virtual bool isCharging() const = 0;
};
