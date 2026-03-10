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
  void updateMetrics(const VisionSnapshot& snapshot);
  void emitMetrics(unsigned long nowMs);

  ICameraPort& cameraPort_;
  EventBus& eventBus_;

  unsigned long lastVisionEventMs_ = 0;
  unsigned long lastCloudRequestMs_ = 0;

  unsigned long metricsWindowStartMs_ = 0;
  uint32_t metricsFrames_ = 0;
  uint32_t metricsRealFrames_ = 0;
  uint32_t metricsSyntheticFrames_ = 0;
  uint32_t metricsCaptureFail_ = 0;
  uint32_t metricsCaptureTotalUs_ = 0;
  uint32_t metricsCaptureMaxUs_ = 0;
};
