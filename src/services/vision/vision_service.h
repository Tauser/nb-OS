#pragma once

#include "../../core/event_bus.h"
#include "../../interfaces/i_camera_port.h"
#include "../../interfaces/i_vision_service.h"
#include "../../models/cloud_types.h"

class VisionService : public IVisionService {
public:
  VisionService(ICameraPort& cameraPort, EventBus& eventBus);

  void init() override;
  void update() override;

private:
  void publishVisionEvent(EventType type, int value, unsigned long nowMs);
  void requestCloud(CloudRequestType requestType, unsigned long nowMs);

  ICameraPort& cameraPort_;
  EventBus& eventBus_;

  unsigned long lastVisionEventMs_ = 0;
  unsigned long lastCloudRequestMs_ = 0;
};
