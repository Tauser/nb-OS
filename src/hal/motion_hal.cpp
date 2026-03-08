#include "motion_hal.h"

#include "../config/hardware_config.h"
#include "../utils/math_utils.h"

MotionHAL::MotionHAL(FeetechBusDriver& motionDriver)
    : motionDriver_(motionDriver) {}

bool MotionHAL::init() {
  return motionDriver_.init();
}

bool MotionHAL::applyPose(const MotionPose& pose) {
  if (!motionDriver_.isReady()) {
    return false;
  }

  const uint16_t yawPos = toServoPosition(
      pose.yawDeg,
      HardwareConfig::Motion::YAW_MIN_DEG,
      HardwareConfig::Motion::YAW_MAX_DEG);

  const uint16_t tiltPos = toServoPosition(
      pose.tiltDeg,
      HardwareConfig::Motion::TILT_MIN_DEG,
      HardwareConfig::Motion::TILT_MAX_DEG);

  const bool yawOk = motionDriver_.writePosition(
      static_cast<uint8_t>(HardwareConfig::Motion::YAW_SERVO_ID),
      yawPos,
      static_cast<uint16_t>(pose.durationMs));

  const bool tiltOk = motionDriver_.writePosition(
      static_cast<uint8_t>(HardwareConfig::Motion::TILT_SERVO_ID),
      tiltPos,
      static_cast<uint16_t>(pose.durationMs));

  return yawOk && tiltOk;
}

bool MotionHAL::isReady() const {
  return motionDriver_.isReady();
}

uint16_t MotionHAL::toServoPosition(float angleDeg, float minDeg, float maxDeg) const {
  const float clamped = MathUtils::clamp(angleDeg, minDeg, maxDeg);
  const float raw = static_cast<float>(HardwareConfig::Motion::SERVO_POS_CENTER) +
                    (clamped * HardwareConfig::Motion::SERVO_POS_PER_DEG);

  const int asInt = MathUtils::clamp(static_cast<int>(raw),
                                     HardwareConfig::Motion::SERVO_POS_MIN,
                                     HardwareConfig::Motion::SERVO_POS_MAX);
  return static_cast<uint16_t>(asInt);
}
