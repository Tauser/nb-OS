#include "gaze_service.h"

#include "../../config/hardware_config.h"
#include "../../models/event.h"
#include <Arduino.h>

GazeService::GazeService(EventBus& eventBus, IFaceController& faceController, IMotion& motion)
    : eventBus_(eventBus), faceController_(faceController), motion_(motion) {}

void GazeService::init() {
  eventBus_.subscribe(EventType::EVT_ATTENTION_CHANGED, this);

  const unsigned long nowMs = millis();
  lastGazeUpdateMs_ = nowMs;
  lastCommandMs_ = nowMs;
}

void GazeService::update(unsigned long) {
  // Event-driven to avoid continuous expression churn in idle.
}

void GazeService::onEvent(const Event& event) {
  if (event.type != EventType::EVT_ATTENTION_CHANGED) {
    return;
  }

  const AttentionFocus newFocus = static_cast<AttentionFocus>(event.value);
  if (newFocus == focus_) {
    return;
  }

  focus_ = newFocus;
  const unsigned long nowMs = (event.timestamp > 0) ? event.timestamp : millis();
  applyFocusGaze(nowMs);
}

void GazeService::applyFocusGaze(unsigned long nowMs) {
  if (nowMs - lastCommandMs_ < HardwareConfig::Polish::GAZE_COMMAND_COOLDOWN_MS) {
    return;
  }
  lastCommandMs_ = nowMs;

  switch (focus_) {
    case AttentionFocus::Touch:
      faceController_.requestExpression(ExpressionType::FaceRecognized, EyeAnimPriority::Social, 500);
      motion_.curiousRight();
      break;

    case AttentionFocus::Voice:
      faceController_.requestExpression(ExpressionType::FaceRecognized, EyeAnimPriority::Social, 550);
      motion_.softListen();
      break;

    case AttentionFocus::Vision:
      faceController_.requestExpression(ExpressionType::Curiosity, EyeAnimPriority::Emotion, 500);
      if (turnLeft_) {
        motion_.curiousLeft();
      } else {
        motion_.curiousRight();
      }
      turnLeft_ = !turnLeft_;
      break;

    case AttentionFocus::Power:
      faceController_.requestExpression(ExpressionType::BatteryAlert, EyeAnimPriority::Emotion, 600);
      motion_.center();
      break;

    case AttentionFocus::Internal:
    case AttentionFocus::Idle:
    case AttentionFocus::None:
    default:
      // Keep face ownership with Routine/Behavior during idle loops.
      motion_.idleSway();
      break;
  }
}
