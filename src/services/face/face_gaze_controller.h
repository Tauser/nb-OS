#pragma once

#include <stdint.h>

#include "../../models/expression_types.h"
#include "../../models/face_gaze_target.h"
#include "../../models/face_render_state.h"

class FaceGazeController {
public:
  struct Output {
    bool enabled = false;
    FaceGazeMode mode = FaceGazeMode::None;
    float currentXNorm = 0.0f;
    float currentYNorm = 0.0f;
    float targetXNorm = 0.0f;
    float targetYNorm = 0.0f;
    float amplitude = 0.0f;
    unsigned long fixationUntilMs = 0;
  };

  void init(unsigned long nowMs);
  void setContext(ExpressionType expression, float attention, float curiosity, bool idleLike);
  void setTarget(const FaceGazeTarget& target, unsigned long nowMs);
  void clearTarget(unsigned long nowMs);
  void update(unsigned long nowMs);

  const Output& output() const;

private:
  enum class Phase : uint8_t {
    Idle = 0,
    ToOvershoot,
    ToSettle,
    Fixation,
    Micro
  };

  struct Durations {
    unsigned long saccadeMs = 90;
    unsigned long settleMs = 100;
    unsigned long fixationMs = 620;
    float overshoot = 0.16f;
  };

  void queueTarget(float xNorm,
                   float yNorm,
                   FaceSaccadeSize saccadeSize,
                   FaceGazeBehavior behavior,
                   unsigned long fixationMs,
                   bool external,
                   unsigned long nowMs);

  void startIdleScan(unsigned long nowMs);
  void startMicroSaccade(unsigned long nowMs);
  void startReturnToCenter(unsigned long nowMs);
  void completePrimaryMotion(unsigned long nowMs);

  Durations durationsFor(FaceSaccadeSize size, float distance) const;
  FaceSaccadeSize resolvedSaccadeSize(FaceSaccadeSize preferred, float distance) const;
  unsigned long microIntervalMs() const;
  float microAmplitude() const;

  static float clampX(float v);
  static float clampY(float v);  static float lerp(float a, float b, float t);
  static float easeOutCubic(float t);
  static float easeInOutCubic(float t);

  Output output_{};

  bool hasExternalTarget_ = false;
  bool pendingReturnToCenter_ = false;
  bool idleLike_ = true;
  bool scanSignPositive_ = true;
  bool microSignPositive_ = true;

  Phase phase_ = Phase::Idle;
  FaceGazeBehavior activeBehavior_ = FaceGazeBehavior::Hold;
  ExpressionType expression_ = ExpressionType::Neutral;

  float contextAttention_ = 0.5f;
  float contextCuriosity_ = 0.5f;

  float currentX_ = 0.0f;
  float currentY_ = 0.0f;

  float phaseFromX_ = 0.0f;
  float phaseFromY_ = 0.0f;
  float phaseToX_ = 0.0f;
  float phaseToY_ = 0.0f;
  float settleToX_ = 0.0f;
  float settleToY_ = 0.0f;

  float microBaseX_ = 0.0f;
  float microBaseY_ = 0.0f;
  float microTargetX_ = 0.0f;
  float microTargetY_ = 0.0f;

  unsigned long phaseStartMs_ = 0;
  unsigned long phaseDurationMs_ = 0;
  unsigned long fixationUntilMs_ = 0;
  unsigned long nextMicroAtMs_ = 0;
  unsigned long nextIdleScanAtMs_ = 0;
};

