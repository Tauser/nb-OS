#pragma once

#include "../interfaces/i_vision_service.h"
#include "../interfaces/i_visual_service.h"
#include "../models/robot_state.h"
#include "config_manager.h"
#include "diagnostics.h"
#include "event_bus.h"
#include "storage_manager.h"

class SystemManager {
public:
  SystemManager(EventBus& eventBus,
                Diagnostics& diagnostics,
                ConfigManager& configManager,
                StorageManager& storageManager,
                IVisualService& visualService,
                IVisionService& visionService);

  void init();
  void update();
  RobotState getState() const;

private:
  void publishEvent(EventType type, EventSource source = EventSource::System, int value = 0);
  const char* getStateName() const;

  EventBus& eventBus_;
  Diagnostics& diagnostics_;
  ConfigManager& configManager_;
  StorageManager& storageManager_;
  IVisualService& visualService_;
  IVisionService& visionService_;

  RobotState state_ = RobotState::Boot;
  unsigned long lastHeartbeatMs_ = 0;
  unsigned long lastFrameMs_ = 0;
};
