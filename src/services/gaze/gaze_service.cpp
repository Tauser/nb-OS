#include "gaze_service.h"

#include "../../config/hardware_config.h"
#include "../../models/event.h"
#include <Arduino.h>

namespace {
FaceGazeTarget makeTarget(float x,
                          float y,
                          FaceSaccadeSize size,
                          FaceGazeBehavior behavior,
                          unsigned long fixationMs,
                          bool microSaccades) {
  FaceGazeTarget out;
  out.enabled = true;
  out.xNorm = x;
  out.yNorm = y;
  out.saccadeSize = size;
  out.behavior = behavior;
  out.fixationMs = fixationMs;
  out.microSaccades = microSaccades;
  out.weight = 1.0f;
  out.sanitize();
  return out;
}
}

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
    case AttentionFocus::Touch: {
      faceController_.requestExpression(ExpressionType::FaceRecognized, EyeAnimPriority::Social, 500);
      const float x = turnLeft_ ? -0.34f : 0.34f;
      faceController_.requestGazeTarget(makeTarget(x,
                                                   0.04f,
                                                   FaceSaccadeSize::Medium,
                                                   FaceGazeBehavior::Hold,
                                                   720,
                                                   true));
      motion_.curiousRight();
      break;
    }

    case AttentionFocus::Voice:
      faceController_.requestExpression(ExpressionType::Listening, EyeAnimPriority::Social, 650);
      faceController_.requestGazeTarget(makeTarget(0.0f,
                                                   -0.10f,
                                                   FaceSaccadeSize::Long,
                                                   FaceGazeBehavior::Hold,
                                                   920,
                                                   true));
      motion_.softListen();
      break;

    case AttentionFocus::Vision: {
      faceController_.requestExpression(ExpressionType::Curiosity, EyeAnimPriority::Emotion, 500);
      const float edgeX = turnLeft_ ? -0.78f : 0.78f;
      faceController_.requestGazeTarget(makeTarget(edgeX,
                                                   -0.04f,
                                                   FaceSaccadeSize::Medium,
                                                   FaceGazeBehavior::EdgePeek,
                                                   340,
                                                   false));
      if (turnLeft_) {
        motion_.curiousLeft();
      } else {
        motion_.curiousRight();
      }
      turnLeft_ = !turnLeft_;
      break;
    }

    case AttentionFocus::Power:
      faceController_.requestExpression(ExpressionType::BatteryAlert, EyeAnimPriority::Emotion, 600);
      faceController_.requestGazeTarget(makeTarget(0.0f,
                                                   0.26f,
                                                   FaceSaccadeSize::Short,
                                                   FaceGazeBehavior::RecoveryGlance,
                                                   480,
                                                   false));
      motion_.center();
      break;

    case AttentionFocus::Internal:
      faceController_.requestGazeTarget(makeTarget(turnLeft_ ? -0.24f : 0.24f,
                                                   -0.02f,
                                                   FaceSaccadeSize::Short,
                                                   FaceGazeBehavior::RecoveryGlance,
                                                   380,
                                                   false));
      turnLeft_ = !turnLeft_;
      motion_.idleSway();
      break;

    case AttentionFocus::Idle:
      faceController_.requestGazeTarget(makeTarget(turnLeft_ ? -0.30f : 0.30f,
                                                   -0.03f,
                                                   FaceSaccadeSize::Medium,
                                                   FaceGazeBehavior::Scan,
                                                   520,
                                                   false));
      turnLeft_ = !turnLeft_;
      motion_.idleSway();
      break;

    case AttentionFocus::None:
    default:
      faceController_.clearGazeTarget();
      motion_.idleSway();
      break;
  }
}
