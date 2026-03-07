#pragma once

#include "../../core/event_bus.h"
#include "../../hal/sensor_hal.h"
#include "../../interfaces/i_sensor_hub.h"
#include "../../models/sensor_snapshot.h"

class SensorService : public ISensorHub {
public:
  SensorService(SensorHAL& sensorHal, EventBus& eventBus);

  void init() override;
  void update(unsigned long nowMs) override;

private:
  void publishEvent(EventType type, int value, unsigned long nowMs);

  SensorHAL& sensorHal_;
  EventBus& eventBus_;
  SensorSnapshot lastSnapshot_{};

  bool hasLastSnapshot_ = false;
  bool lastTouchState_ = false;

  unsigned long lastTouchEventMs_ = 0;
  unsigned long lastShakeEventMs_ = 0;
  unsigned long lastTiltEventMs_ = 0;
  unsigned long lastFallEventMs_ = 0;
};
