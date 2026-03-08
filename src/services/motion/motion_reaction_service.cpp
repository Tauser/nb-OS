#include "motion_reaction_service.h"

#include "../../config/hardware_config.h"
#include <math.h>

namespace {
float clamp01(float value) {
  if (value < 0.0f) return 0.0f;
  if (value > 1.0f) return 1.0f;
  return value;
}

float applyDeadzone(float value, float deadzone) {
  const float x = clamp01(value);
  const float dz = clamp01(deadzone);
  if (x <= dz) {
    return 0.0f;
  }
  return (x - dz) / (1.0f - dz);
}

float lerp(float a, float b, float t) {
  return a + ((b - a) * t);
}

float easeOutPower(float t, float power) {
  const float x = clamp01(t);
  return 1.0f - powf(1.0f - x, power);
}

float smoothstep(float t) {
  const float x = clamp01(t);
  return x * x * (3.0f - (2.0f * x));
}

unsigned long lerpDuration(unsigned long minMs, unsigned long maxMs, float t) {
  return static_cast<unsigned long>(lerp(static_cast<float>(maxMs), static_cast<float>(minMs), t));
}

MotionPose makePose(MotionPoseId id, float yawDeg, float tiltDeg, unsigned long durationMs) {
  MotionPose pose;
  pose.id = id;
  pose.yawDeg = yawDeg;
  pose.tiltDeg = tiltDeg;
  pose.durationMs = durationMs;
  return pose;
}
}

MotionReactionService::MotionReactionService(EventBus& eventBus, IMotion& motion)
    : eventBus_(eventBus), motion_(motion) {}

void MotionReactionService::init() {
  eventBus_.subscribe(EventType::EVT_TOUCH, this);
  eventBus_.subscribe(EventType::EVT_SHAKE, this);
  eventBus_.subscribe(EventType::EVT_TILT, this);
  eventBus_.subscribe(EventType::EVT_FALL, this);
}

void MotionReactionService::update(unsigned long nowMs) {
  (void)nowMs;
}

void MotionReactionService::onEvent(const Event& event) {
  const unsigned long cooldownMs = HardwareConfig::MotionReactions::REACTION_COOLDOWN_MS;
  if (event.timestamp - lastReactionMs_ < cooldownMs) {
    return;
  }

  switch (event.type) {
    case EventType::EVT_TOUCH: {
      const int excess = event.value - HardwareConfig::Touch::ANALOG_TOUCH_THRESHOLD;
      const float raw = clamp01(static_cast<float>(excess) /
                                static_cast<float>(HardwareConfig::MotionReactions::TOUCH_EXCESS_MAX));
      const float active = applyDeadzone(raw, HardwareConfig::MotionReactions::TOUCH_DEADZONE);
      const float intensity = smoothstep(easeOutPower(active, HardwareConfig::MotionReactions::TOUCH_EASE_POWER));
      if (intensity <= 0.0f) {
        break;
      }

      const float yawAbs = lerp(HardwareConfig::MotionReactions::TOUCH_MIN_YAW_DEG,
                                HardwareConfig::MotionReactions::TOUCH_MAX_YAW_DEG,
                                intensity);
      const float tilt = lerp(HardwareConfig::MotionReactions::TOUCH_MIN_TILT_DEG,
                              HardwareConfig::MotionReactions::TOUCH_MAX_TILT_DEG,
                              intensity);
      const unsigned long duration = lerpDuration(HardwareConfig::MotionReactions::REACTION_MIN_MS,
                                                  HardwareConfig::MotionReactions::REACTION_MAX_MS,
                                                  intensity);

      const bool toLeft = curiousLeftNext_;
      curiousLeftNext_ = !curiousLeftNext_;

      const MotionPose pose = makePose(toLeft ? MotionPoseId::CuriousLeft : MotionPoseId::CuriousRight,
                                       toLeft ? -yawAbs : yawAbs,
                                       tilt,
                                       duration);
      motion_.applyPose(pose);
      lastReactionMs_ = event.timestamp;
      break;
    }

    case EventType::EVT_SHAKE: {
      const float raw = clamp01(static_cast<float>(event.value) /
                                static_cast<float>(HardwareConfig::MotionReactions::SHAKE_MG_MAX));
      const float active = applyDeadzone(raw, HardwareConfig::MotionReactions::SHAKE_DEADZONE);
      const float intensity = smoothstep(easeOutPower(active, HardwareConfig::MotionReactions::SHAKE_EASE_POWER));
      if (intensity <= 0.0f) {
        break;
      }

      const float yawAbs = lerp(4.0f, HardwareConfig::Motion::IDLE_SWAY_YAW_DEG, intensity);
      const unsigned long duration = lerpDuration(HardwareConfig::MotionReactions::REACTION_MIN_MS,
                                                  HardwareConfig::MotionReactions::REACTION_MAX_MS,
                                                  intensity);
      const MotionPose pose = makePose(MotionPoseId::IdleSway,
                                       curiousLeftNext_ ? -yawAbs : yawAbs,
                                       0.0f,
                                       duration);
      curiousLeftNext_ = !curiousLeftNext_;
      motion_.applyPose(pose);
      motion_.idleSway();
      lastReactionMs_ = event.timestamp;
      break;
    }

    case EventType::EVT_TILT: {
      const float raw = clamp01(static_cast<float>(event.value) /
                                static_cast<float>(HardwareConfig::MotionReactions::TILT_MG_MAX));
      const float active = applyDeadzone(raw, HardwareConfig::MotionReactions::TILT_DEADZONE);
      const float intensity = smoothstep(easeOutPower(active, HardwareConfig::MotionReactions::TILT_EASE_POWER));
      if (intensity <= 0.0f) {
        break;
      }

      const float tilt = lerp(HardwareConfig::MotionReactions::TILT_MIN_DEG,
                              HardwareConfig::MotionReactions::TILT_MAX_DEG,
                              intensity);
      const unsigned long duration = lerpDuration(HardwareConfig::MotionReactions::REACTION_MIN_MS,
                                                  HardwareConfig::MotionReactions::REACTION_MAX_MS,
                                                  intensity);
      const MotionPose pose = makePose(MotionPoseId::SoftListen, 0.0f, tilt, duration);
      motion_.applyPose(pose);
      lastReactionMs_ = event.timestamp;
      break;
    }

    case EventType::EVT_FALL: {
      // Lower abs-Z means stronger fall event.
      const float normalizedZ = clamp01(static_cast<float>(event.value) /
                                        static_cast<float>(HardwareConfig::MotionReactions::FALL_MG_MAX));
      const float rawIntensity = 1.0f - normalizedZ;
      const float active = applyDeadzone(rawIntensity, HardwareConfig::MotionReactions::FALL_DEADZONE);
      const float intensity = smoothstep(easeOutPower(active, HardwareConfig::MotionReactions::FALL_EASE_POWER));
      if (intensity <= 0.0f) {
        break;
      }

      const unsigned long duration = lerpDuration(HardwareConfig::MotionReactions::REACTION_MIN_MS,
                                                  HardwareConfig::MotionReactions::REACTION_MAX_MS,
                                                  intensity);
      const MotionPose pose = makePose(MotionPoseId::Center, 0.0f, 0.0f, duration);
      motion_.applyPose(pose);
      lastReactionMs_ = event.timestamp;
      break;
    }

    default:
      break;
  }
}
