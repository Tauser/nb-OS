#pragma once

#include "../models/motion_pose.h"

class IMotionStateProvider {
public:
  virtual ~IMotionStateProvider() = default;

  virtual float getCurrentYawDeg() const = 0;
  virtual float getCurrentTiltDeg() const = 0;
  virtual MotionPoseId getCurrentPoseId() const = 0;
};
