#pragma once

#include "../interfaces/i_event_listener.h"
#include "../interfaces/i_motion.h"
#include "../interfaces/i_sensor_hub.h"
#include "../interfaces/i_vision_service.h"
#include "../interfaces/i_visual_service.h"
#include "../models/power_state.h"
#include "../models/robot_state.h"
#include "config_manager.h"
#include "diagnostics.h"
#include "event_bus.h"
#include "storage_manager.h"

class SystemManager : public IEventListener {
public:
  SystemManager(EventBus& eventBus,
                Diagnostics& diagnostics,
                ConfigManager& configManager,
                StorageManager& storageManager,
                IVisualService& visualService,
                IVisionService& visionService,
                ISensorHub& sensorHub,
                IMotion& motionService);

  void init();
  void update();
  RobotState getState() const;
  void onEvent(const Event& event) override;

private:
  void publishEvent(EventType type, EventSource source = EventSource::System, int value = 0);
  const char* getStateName() const;
  unsigned long scaledInterval(unsigned long baseMs, float scale) const;

  EventBus& eventBus_;
  Diagnostics& diagnostics_;
  ConfigManager& configManager_;
  StorageManager& storageManager_;
  IVisualService& visualService_;
  IVisionService& visionService_;
  ISensorHub& sensorHub_;
  IMotion& motionService_;

  RobotState state_ = RobotState::Boot;
  PowerMode powerMode_ = PowerMode::Normal;
  unsigned long lastHeartbeatMs_ = 0;
  unsigned long lastFrameMs_ = 0;
  unsigned long lastSensorPollMs_ = 0;
  unsigned long lastMotionUpdateMs_ = 0;
};
