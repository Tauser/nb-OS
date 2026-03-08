#pragma once

#include "../drivers/motion/feetech_bus_driver.h"
#include "../models/motion_pose.h"
#include <stdint.h>

class MotionHAL {
public:
  explicit MotionHAL(FeetechBusDriver& motionDriver);

  bool init();
  bool applyPose(const MotionPose& pose);
  bool isReady() const;

private:
  uint16_t toServoPosition(float angleDeg, float minDeg, float maxDeg) const;

  FeetechBusDriver& motionDriver_;
};
