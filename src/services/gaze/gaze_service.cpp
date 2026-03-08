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

void GazeService::update(unsigned long nowMs) {
  if (nowMs - lastGazeUpdateMs_ < HardwareConfig::Polish::GAZE_UPDATE_INTERVAL_MS) {
    return;
  }

  lastGazeUpdateMs_ = nowMs;
  applyFocusGaze(nowMs);
}

void GazeService::onEvent(const Event& event) {
  if (event.type != EventType::EVT_ATTENTION_CHANGED) {
    return;
  }

  focus_ = static_cast<AttentionFocus>(event.value);
}

void GazeService::applyFocusGaze(unsigned long nowMs) {
  if (nowMs - lastCommandMs_ < HardwareConfig::Polish::GAZE_COMMAND_COOLDOWN_MS) {
    return;
  }
  lastCommandMs_ = nowMs;

  switch (focus_) {
    case AttentionFocus::Touch:
      faceController_.requestExpression(ExpressionType::FaceRecognized, EyeAnimPriority::Social, 400);
      motion_.curiousRight();
      break;

    case AttentionFocus::Voice:
      faceController_.requestExpression(ExpressionType::FaceRecognized, EyeAnimPriority::Social, 450);
      motion_.softListen();
      break;

    case AttentionFocus::Vision:
      faceController_.requestExpression(ExpressionType::Curiosity, EyeAnimPriority::Emotion, 420);
      if (turnLeft_) {
        motion_.curiousLeft();
      } else {
        motion_.curiousRight();
      }
      turnLeft_ = !turnLeft_;
      break;

    case AttentionFocus::Power:
      faceController_.requestExpression(ExpressionType::BatteryAlert, EyeAnimPriority::Emotion, 500);
      motion_.center();
      break;

    case AttentionFocus::Internal:
      faceController_.requestExpression(ExpressionType::Curiosity, EyeAnimPriority::Idle, 350);
      motion_.idleSway();
      break;

    case AttentionFocus::Idle:
    case AttentionFocus::None:
    default:
      faceController_.requestExpression(ExpressionType::Neutral, EyeAnimPriority::Idle, 300);
      motion_.idleSway();
      break;
  }
}
