#include "motion_sync_service.h"

#include "../../config/hardware_config.h"
#include <Arduino.h>

MotionSyncService::MotionSyncService(EventBus& eventBus, IFaceController& faceController, IMotion& motion)
    : eventBus_(eventBus), faceController_(faceController), motion_(motion) {}

void MotionSyncService::init() {
  eventBus_.subscribe(EventType::EVT_VOICE_START, this);
  eventBus_.subscribe(EventType::EVT_VOICE_END, this);
  eventBus_.subscribe(EventType::EVT_EMOTION_CHANGED, this);

  lastSyncMs_ = millis();
}

void MotionSyncService::update(unsigned long) {
}

void MotionSyncService::onEvent(const Event& event) {
  const unsigned long nowMs = (event.timestamp > 0) ? event.timestamp : millis();
  if (nowMs - lastSyncMs_ < HardwareConfig::Polish::MOTION_SYNC_COOLDOWN_MS) {
    return;
  }

  switch (event.type) {
    case EventType::EVT_VOICE_START:
      faceController_.requestExpression(ExpressionType::FaceRecognized, EyeAnimPriority::Social, 520);
      motion_.softListen();
      lastSyncMs_ = nowMs;
      break;

    case EventType::EVT_VOICE_END:
      motion_.idleSway();
      lastSyncMs_ = nowMs;
      break;

    case EventType::EVT_EMOTION_CHANGED:
      if (event.value > 700) {
        if (yawLeft_) {
          motion_.yawLeft();
        } else {
          motion_.yawRight();
        }
        yawLeft_ = !yawLeft_;
        lastSyncMs_ = nowMs;
      }
      break;

    default:
      break;
  }
}
