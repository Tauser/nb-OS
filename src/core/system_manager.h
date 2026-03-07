#pragma once

#include "../models/robot_state.h"
#include "event_bus.h"
#include "diagnostics.h"
#include "config_manager.h"
#include "storage_manager.h"
#include "../services/face/face_service.h"
#include "../services/vision/vision_service.h"

class SystemManager {
public:
  SystemManager(EventBus& eventBus,
                Diagnostics& diagnostics,
                ConfigManager& configManager,
                StorageManager& storageManager,
                FaceService& faceService,
                VisionService& visionService);

  void init();
  void update();
  RobotState getState() const;

private:
  void publishEvent(EventType type, int value = 0);
  const char* getStateName() const;

  EventBus& eventBus_;
  Diagnostics& diagnostics_;
  ConfigManager& configManager_;
  StorageManager& storageManager_;
  FaceService& faceService_;
  VisionService& visionService_;

  RobotState state_ = RobotState::Boot;
  unsigned long lastHeartbeatMs_ = 0;
  unsigned long lastFrameMs_ = 0;
};