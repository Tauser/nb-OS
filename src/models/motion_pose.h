#pragma once

enum class MotionPoseId {
  Center = 0,
  YawLeft,
  YawRight,
  TiltLeft,
  TiltRight,
  CuriousLeft,
  CuriousRight,
  SoftListen,
  IdleSway
};

struct MotionPose {
  MotionPoseId id = MotionPoseId::Center;
  float yawDeg = 0.0f;
  float tiltDeg = 0.0f;
  unsigned long durationMs = 320;
};

namespace MotionPoses {
MotionPose center();
MotionPose yawLeft();
MotionPose yawRight();
MotionPose tiltLeft();
MotionPose tiltRight();
MotionPose curiousLeft();
MotionPose curiousRight();
MotionPose softListen();
MotionPose idleSway(float signedYawDeg);
}
