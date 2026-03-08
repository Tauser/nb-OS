#pragma once

#include "../../core/event_bus.h"
#include "../../hal/power_hal.h"
#include "../../interfaces/i_power.h"

class PowerService : public IPower {
public:
  PowerService(PowerHAL& powerHal, EventBus& eventBus);

  void init() override;
  void update(unsigned long nowMs) override;

  int getBatteryPercent() const override;
  bool isCharging() const override;
  PowerMode getPowerMode() const override;
  const PowerState& getPowerState() const override;

private:
  int batteryPercentFromMillivolts(int mv) const;
  PowerMode decideMode(int batteryPercent, bool charging) const;
  void publishStatusEvent(unsigned long nowMs);
  void publishChargingEvent(bool charging, unsigned long nowMs);
  void publishModeEvent(PowerMode mode, unsigned long nowMs);

  PowerHAL& powerHal_;
  EventBus& eventBus_;

  PowerState state_{};
  unsigned long lastSampleMs_ = 0;
  unsigned long lastStatusPublishMs_ = 0;
};
