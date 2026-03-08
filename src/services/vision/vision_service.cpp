#include "vision_service.h"

#include "../../config/feature_flags.h"
#include "../../config/hardware_config.h"
#include "../../models/cloud_types.h"
#include "../../models/event.h"

VisionService::VisionService(ICameraPort& cameraPort, EventBus& eventBus)
    : cameraPort_(cameraPort), eventBus_(eventBus) {}

void VisionService::init() {
  cameraPort_.init();
}

void VisionService::update() {
  VisionSnapshot snapshot;
  if (!cameraPort_.sampleFrame(snapshot) || !snapshot.valid) {
    return;
  }

  const unsigned long nowMs = snapshot.timestampMs;
  publishVisionEvent(EventType::EVT_VISION_SNAPSHOT, static_cast<int>(snapshot.avgLuma), nowMs);

  if (snapshot.avgLuma <= HardwareConfig::Vision::DARK_LUMA_THRESHOLD &&
      nowMs - lastVisionEventMs_ >= HardwareConfig::Vision::EVENT_COOLDOWN_MS) {
    publishVisionEvent(EventType::EVT_VISION_DARK, static_cast<int>(snapshot.avgLuma), nowMs);
    requestCloud(CloudRequestType::VisionDark, nowMs);
    lastVisionEventMs_ = nowMs;
  }

  if (snapshot.motionScore >= HardwareConfig::Vision::MOTION_SCORE_THRESHOLD &&
      nowMs - lastVisionEventMs_ >= HardwareConfig::Vision::EVENT_COOLDOWN_MS) {
    publishVisionEvent(EventType::EVT_VISION_MOTION, static_cast<int>(snapshot.motionScore), nowMs);
    requestCloud(CloudRequestType::VisionMotion, nowMs);
    lastVisionEventMs_ = nowMs;
  }
}

void VisionService::publishVisionEvent(EventType type, int value, unsigned long nowMs) {
  Event event;
  event.type = type;
  event.source = EventSource::VisionService;
  event.value = value;
  event.timestamp = nowMs;
  eventBus_.publish(event);
}

void VisionService::requestCloud(CloudRequestType requestType, unsigned long nowMs) {
  if (!FeatureFlags::CLOUD_ENABLED) {
    return;
  }

  if (nowMs - lastCloudRequestMs_ < HardwareConfig::Cloud::REQUEST_COOLDOWN_MS) {
    return;
  }

  Event event;
  event.type = EventType::EVT_CLOUD_REQUEST;
  event.source = EventSource::VisionService;
  event.value = static_cast<int>(requestType);
  event.timestamp = nowMs;
  eventBus_.publish(event);

  lastCloudRequestMs_ = nowMs;
}
