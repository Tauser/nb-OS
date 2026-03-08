#include "motion_service.h"

#include "../../config/hardware_config.h"
#include "../../models/event.h"
#include "../../utils/math_utils.h"
#include <Arduino.h>

MotionService::MotionService(MotionHAL& motionHal, EventBus& eventBus, const IEmotionProvider& emotionProvider)
    : motionHal_(motionHal), eventBus_(eventBus), emotionProvider_(emotionProvider) {}

void MotionService::init() {
  motionHal_.init();
  const unsigned long now = millis();
  idleIntervalMs_ = HardwareConfig::Motion::IDLE_SWAY_INTERVAL_MS;
  center();
  lastIdleStepMs_ = now;
  lastEmotionOutputMs_ = now;
}

void MotionService::update(unsigned long nowMs) {
  applyEmotionOutput(nowMs);

  if (!idleEnabled_) {
    return;
  }

  if (nowMs - lastIdleStepMs_ < idleIntervalMs_) {
    return;
  }

  lastIdleStepMs_ = nowMs;
  const float baseYaw = idleSideLeft_
                            ? -HardwareConfig::Motion::IDLE_SWAY_YAW_DEG
                            : HardwareConfig::Motion::IDLE_SWAY_YAW_DEG;
  idleSideLeft_ = !idleSideLeft_;

  const float signedYaw = baseYaw * emotionYawScale_;
  MotionPose pose = MotionPoses::idleSway(signedYaw);
  pose.tiltDeg = emotionTiltBiasDeg_;
  applyPoseInternal(pose, nowMs);
}

void MotionService::center() {
  idleEnabled_ = true;
  applyPoseInternal(MotionPoses::center(), millis());
}

void MotionService::yawLeft() {
  idleEnabled_ = false;
  applyPoseInternal(MotionPoses::yawLeft(), millis());
}

void MotionService::yawRight() {
  idleEnabled_ = false;
  applyPoseInternal(MotionPoses::yawRight(), millis());
}

void MotionService::tiltLeft() {
  idleEnabled_ = false;
  applyPoseInternal(MotionPoses::tiltLeft(), millis());
}

void MotionService::tiltRight() {
  idleEnabled_ = false;
  applyPoseInternal(MotionPoses::tiltRight(), millis());
}

void MotionService::curiousLeft() {
  idleEnabled_ = false;
  applyPoseInternal(MotionPoses::curiousLeft(), millis());
}

void MotionService::curiousRight() {
  idleEnabled_ = false;
  applyPoseInternal(MotionPoses::curiousRight(), millis());
}

void MotionService::softListen() {
  idleEnabled_ = false;
  applyPoseInternal(MotionPoses::softListen(), millis());
}

void MotionService::idleSway() {
  idleEnabled_ = true;
  lastIdleStepMs_ = millis();
}

bool MotionService::applyPose(const MotionPose& pose) {
  idleEnabled_ = false;
  return applyPoseInternal(pose, millis());
}

void MotionService::applyEmotionOutput(unsigned long nowMs) {
  if (nowMs - lastEmotionOutputMs_ < HardwareConfig::EmotionOutput::MOTION_APPLY_INTERVAL_MS) {
    return;
  }
  lastEmotionOutputMs_ = nowMs;

  const EmotionState& emo = emotionProvider_.getEmotionState();

  const float activeBoost = emo.arousal * HardwareConfig::EmotionOutput::MOTION_ACTIVE_INTERVAL_SCALE;
  const float sleepyBoost = (1.0f - emo.energy) * HardwareConfig::EmotionOutput::MOTION_SLEEPY_INTERVAL_SCALE;

  const float yawScale = 1.0f + (emo.arousal - 0.5f) -
                         ((1.0f - emo.energy) * HardwareConfig::EmotionOutput::MOTION_SLEEPY_YAW_DAMP_MAX);
  emotionYawScale_ = MathUtils::clamp(yawScale,
                                      HardwareConfig::EmotionOutput::MOTION_ACTIVE_YAW_SCALE_MIN,
                                      HardwareConfig::EmotionOutput::MOTION_ACTIVE_YAW_SCALE_MAX);

  // Curiosity tilts slightly forward, low energy tends to a softer listening tilt.
  emotionTiltBiasDeg_ = (-emo.curiosity * HardwareConfig::EmotionOutput::MOTION_CURIOUS_TILT_DEG_MAX) +
                        ((1.0f - emo.energy) * HardwareConfig::EmotionOutput::MOTION_SLEEPY_TILT_DEG_MAX);

  const float intervalScale = 1.0f - activeBoost + sleepyBoost;
  const unsigned long interval = static_cast<unsigned long>(
      static_cast<float>(HardwareConfig::Motion::IDLE_SWAY_INTERVAL_MS) * intervalScale);
  idleIntervalMs_ = MathUtils::clamp(interval,
                                     HardwareConfig::EmotionOutput::MOTION_INTERVAL_MIN_MS,
                                     HardwareConfig::EmotionOutput::MOTION_INTERVAL_MAX_MS);
}

bool MotionService::applyPoseInternal(const MotionPose& pose, unsigned long nowMs) {
  if (!motionHal_.applyPose(pose)) {
    return false;
  }

  publishPoseEvent(pose.id, nowMs);
  return true;
}

void MotionService::publishPoseEvent(MotionPoseId poseId, unsigned long nowMs) {
  Event event;
  event.type = EventType::EVT_MOTION_POSE_APPLIED;
  event.source = EventSource::MotionService;
  event.value = static_cast<int>(poseId);
  event.timestamp = nowMs;
  eventBus_.publish(event);
}
