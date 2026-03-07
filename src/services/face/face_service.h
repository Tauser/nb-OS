#pragma once

#include "../../interfaces/i_display_port.h"
#include "face_renderer.h"
#include "eye_model.h"
#include "../../interfaces/i_visual_service.h"

class FaceService : public IVisualService {
public:
  explicit FaceService(IDisplayPort& displayPort);

  void init() override;
  void update(unsigned long nowMs) override;

  void requestExpression(ExpressionType expression, EyeAnimPriority priority, unsigned long holdMs = 0);

private:
  struct PerfStats {
    unsigned long windowStartMs = 0;
    unsigned long updates = 0;
    unsigned long renderedFrames = 0;
    unsigned long droppedFrames = 0;
    unsigned long maxRenderUs = 0;
    unsigned long avgRenderUsAccum = 0;
  };

  void updateBlink(unsigned long nowMs);
  void updateIdleMotion(unsigned long nowMs);
  void scheduleNextBlink(unsigned long nowMs);
  void applyStateToTargets();
  void stepStateMachine(unsigned long nowMs);
  void smoothEyes(unsigned long nowMs);
  void recordPerf(unsigned long nowMs, bool rendered, unsigned long renderUs);
  float easeInOutCubic(float t) const;

  IDisplayPort& displayPort_;
  FaceRenderer renderer_;

  EyeModel leftEye_;
  EyeModel rightEye_;
  EyeModel targetLeftEye_;
  EyeModel targetRightEye_;

  ExpressionType currentExpression_ = ExpressionType::Neutral;
  ExpressionType requestedExpression_ = ExpressionType::Neutral;
  EyeAnimPriority currentPriority_ = EyeAnimPriority::Idle;
  EyeAnimPriority requestedPriority_ = EyeAnimPriority::Idle;
  unsigned long expressionHoldUntilMs_ = 0;

  unsigned long nextBlinkAtMs_ = 0;
  unsigned long blinkStartMs_ = 0;
  bool blinking_ = false;

  unsigned long lastIdleMotionMs_ = 0;
  unsigned long lastSmoothMs_ = 0;
  float baseOpenness_ = 1.0f;

  PerfStats perf_;
};
