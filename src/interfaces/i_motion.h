#pragma once

#include "../models/motion_pose.h"

class IMotion {
public:
  virtual ~IMotion() = default;

  virtual void init() = 0;
  virtual void update(unsigned long nowMs) = 0;

  virtual void center() = 0;
  virtual void yawLeft() = 0;
  virtual void yawRight() = 0;
  virtual void tiltLeft() = 0;
  virtual void tiltRight() = 0;
  virtual void curiousLeft() = 0;
  virtual void curiousRight() = 0;
  virtual void softListen() = 0;
  virtual void idleSway() = 0;

  virtual bool applyPose(const MotionPose& pose) = 0;
};
