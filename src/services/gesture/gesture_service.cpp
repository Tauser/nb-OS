#include "gesture_service.h"

#include "../../config/hardware_config.h"
#include "../../models/attention_focus.h"
#include "../../models/event.h"
#include <Arduino.h>

GestureService::GestureService(EventBus& eventBus, IFaceController& faceController, IMotion& motion)
    : eventBus_(eventBus), faceController_(faceController), motion_(motion) {}

void GestureService::init() {
  eventBus_.subscribe(EventType::EVT_TOUCH, this);
  eventBus_.subscribe(EventType::EVT_SHAKE, this);
  eventBus_.subscribe(EventType::EVT_VOICE_START, this);
  eventBus_.subscribe(EventType::EVT_ATTENTION_CHANGED, this);

  lastGestureMs_ = millis();
}

void GestureService::update(unsigned long) {
}

void GestureService::onEvent(const Event& event) {
  const unsigned long nowMs = (event.timestamp > 0) ? event.timestamp : millis();

  switch (event.type) {
    case EventType::EVT_TOUCH:
      playGesture(GestureType::HappyTilt, nowMs);
      break;

    case EventType::EVT_SHAKE:
      playGesture(GestureType::SurprisedJerk, nowMs);
      break;

    case EventType::EVT_VOICE_START:
      playGesture(GestureType::SoftListen, nowMs);
      break;

    case EventType::EVT_ATTENTION_CHANGED:
      if (static_cast<AttentionFocus>(event.value) == AttentionFocus::Vision) {
        playGesture(GestureType::CuriousLeft, nowMs);
      }
      break;

    default:
      break;
  }
}

bool GestureService::playGesture(GestureType gesture, unsigned long nowMs) {
  if (gesture == GestureType::None) {
    return false;
  }

  if (nowMs - lastGestureMs_ < HardwareConfig::Polish::GESTURE_COOLDOWN_MS) {
    return false;
  }

  switch (gesture) {
    case GestureType::CuriousLeft:
      faceController_.requestExpression(ExpressionType::Curiosity, EyeAnimPriority::Social, 420);
      motion_.curiousLeft();
      break;

    case GestureType::HappyTilt:
      faceController_.requestExpression(ExpressionType::FaceRecognized, EyeAnimPriority::Social, 480);
      motion_.tiltRight();
      break;

    case GestureType::ShyTurn:
      faceController_.requestExpression(ExpressionType::Curiosity, EyeAnimPriority::Emotion, 380);
      motion_.yawRight();
      break;

    case GestureType::SoftListen:
      faceController_.requestExpression(ExpressionType::FaceRecognized, EyeAnimPriority::Social, 520);
      motion_.softListen();
      break;

    case GestureType::SurprisedJerk:
      faceController_.requestExpression(ExpressionType::BatteryAlert, EyeAnimPriority::Alert, 300);
      motion_.center();
      break;

    default:
      return false;
  }

  lastGestureMs_ = nowMs;
  publishGesture(gesture, nowMs);
  return true;
}

void GestureService::publishGesture(GestureType gesture, unsigned long nowMs) {
  Event event;
  event.type = EventType::EVT_GESTURE_PLAYED;
  event.source = EventSource::GestureService;
  event.value = static_cast<int>(gesture);
  event.timestamp = nowMs;
  eventBus_.publish(event);
}
