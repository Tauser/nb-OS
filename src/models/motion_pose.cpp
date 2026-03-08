#include "motion_pose.h"

#include "../config/hardware_config.h"

namespace {
MotionPose makePose(MotionPoseId id, float yawDeg, float tiltDeg, unsigned long durationMs) {
  MotionPose pose;
  pose.id = id;
  pose.yawDeg = yawDeg;
  pose.tiltDeg = tiltDeg;
  pose.durationMs = durationMs;
  return pose;
}
}

namespace MotionPoses {
MotionPose center() {
  return makePose(MotionPoseId::Center, 0.0f, 0.0f, HardwareConfig::Motion::DEFAULT_MOVE_MS);
}

MotionPose yawLeft() {
  return makePose(MotionPoseId::YawLeft,
                  -HardwareConfig::Motion::YAW_LOOK_DEG,
                  0.0f,
                  HardwareConfig::Motion::QUICK_MOVE_MS);
}

MotionPose yawRight() {
  return makePose(MotionPoseId::YawRight,
                  HardwareConfig::Motion::YAW_LOOK_DEG,
                  0.0f,
                  HardwareConfig::Motion::QUICK_MOVE_MS);
}

MotionPose tiltLeft() {
  return makePose(MotionPoseId::TiltLeft,
                  0.0f,
                  -HardwareConfig::Motion::TILT_LOOK_DEG,
                  HardwareConfig::Motion::QUICK_MOVE_MS);
}

MotionPose tiltRight() {
  return makePose(MotionPoseId::TiltRight,
                  0.0f,
                  HardwareConfig::Motion::TILT_LOOK_DEG,
                  HardwareConfig::Motion::QUICK_MOVE_MS);
}

MotionPose curiousLeft() {
  return makePose(MotionPoseId::CuriousLeft,
                  -HardwareConfig::Motion::CURIOUS_YAW_DEG,
                  HardwareConfig::Motion::CURIOUS_TILT_DEG,
                  HardwareConfig::Motion::DEFAULT_MOVE_MS);
}

MotionPose curiousRight() {
  return makePose(MotionPoseId::CuriousRight,
                  HardwareConfig::Motion::CURIOUS_YAW_DEG,
                  HardwareConfig::Motion::CURIOUS_TILT_DEG,
                  HardwareConfig::Motion::DEFAULT_MOVE_MS);
}

MotionPose softListen() {
  return makePose(MotionPoseId::SoftListen,
                  0.0f,
                  HardwareConfig::Motion::SOFT_LISTEN_TILT_DEG,
                  HardwareConfig::Motion::DEFAULT_MOVE_MS);
}

MotionPose idleSway(float signedYawDeg) {
  return makePose(MotionPoseId::IdleSway,
                  signedYawDeg,
                  0.0f,
                  HardwareConfig::Motion::IDLE_SWAY_MOVE_MS);
}
}
