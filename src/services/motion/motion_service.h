#pragma once

#include "../../core/event_bus.h"
#include "../../hal/motion_hal.h"
#include "../../interfaces/i_emotion_provider.h"
#include "../../interfaces/i_motion.h"
#include "../../interfaces/i_motion_state_provider.h"
#include "../../models/motion_pose.h"

class MotionService : public IMotion, public IMotionStateProvider {
public:
  MotionService(MotionHAL& motionHal, EventBus& eventBus, const IEmotionProvider& emotionProvider);

  void init() override;
  void update(unsigned long nowMs) override;

  void center() override;
  void yawLeft() override;
  void yawRight() override;
  void tiltLeft() override;
  void tiltRight() override;
  void curiousLeft() override;
  void curiousRight() override;
  void softListen() override;
  void idleSway() override;
  bool applyPose(const MotionPose& pose) override;

  float getCurrentYawDeg() const override;
  float getCurrentTiltDeg() const override;
  MotionPoseId getCurrentPoseId() const override;

private:
  void applyEmotionOutput(unsigned long nowMs);
  bool applyPoseInternal(const MotionPose& pose, unsigned long nowMs);
  void publishPoseEvent(MotionPoseId poseId, unsigned long nowMs);

  MotionHAL& motionHal_;
  EventBus& eventBus_;
  const IEmotionProvider& emotionProvider_;

  bool idleEnabled_ = true;
  bool idleSideLeft_ = true;
  unsigned long lastIdleStepMs_ = 0;
  unsigned long lastEmotionOutputMs_ = 0;
  unsigned long idleIntervalMs_ = 0;
  float emotionYawScale_ = 1.0f;
  float emotionTiltBiasDeg_ = 0.0f;

  float currentYawDeg_ = 0.0f;
  float currentTiltDeg_ = 0.0f;
  MotionPoseId currentPoseId_ = MotionPoseId::Center;
};


