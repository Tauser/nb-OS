#include "face_gaze_controller.h"

#include "../../config/hardware_config.h"
#include "../../utils/math_utils.h"
#include <math.h>

namespace {
constexpr float kDeadzone = 0.03f;
constexpr float kCenterDamp = 0.10f;
}

void FaceGazeController::init(unsigned long nowMs) {
  output_ = Output{};
  phase_ = Phase::Idle;
  currentX_ = 0.0f;
  currentY_ = 0.0f;
  phaseStartMs_ = nowMs;
  nextMicroAtMs_ = nowMs + HardwareConfig::FaceGaze::MICRO_INTERVAL_DEFAULT_MS;
  nextIdleScanAtMs_ = nowMs + HardwareConfig::FaceGaze::IDLE_SCAN_INTERVAL_MS;
}

void FaceGazeController::setContext(ExpressionType expression,
                                    float attention,
                                    float curiosity,
                                    bool idleLike) {
  expression_ = expression;
  contextAttention_ = MathUtils::clamp(attention, 0.0f, 1.0f);
  contextCuriosity_ = MathUtils::clamp(curiosity, 0.0f, 1.0f);
  idleLike_ = idleLike;
}

void FaceGazeController::setTarget(const FaceGazeTarget& target, unsigned long nowMs) {
  FaceGazeTarget safe = target;
  safe.sanitize();

  if (!safe.enabled) {
    clearTarget(nowMs);
    return;
  }

  hasExternalTarget_ = true;
  queueTarget(safe.xNorm,
              safe.yNorm,
              safe.saccadeSize,
              safe.behavior,
              safe.fixationMs,
              true,
              nowMs);
}

void FaceGazeController::clearTarget(unsigned long nowMs) {
  hasExternalTarget_ = false;
  pendingReturnToCenter_ = false;
  queueTarget(0.0f,
              0.0f,
              FaceSaccadeSize::Short,
              FaceGazeBehavior::RecoveryGlance,
              HardwareConfig::FaceGaze::FIXATION_SHORT_MS,
              false,
              nowMs);
}

void FaceGazeController::update(unsigned long nowMs) {
  if (!hasExternalTarget_ && phase_ == Phase::Idle) {
    if (idleLike_ && nowMs >= nextIdleScanAtMs_) {
      startIdleScan(nowMs);
    }

    currentX_ = lerp(currentX_, 0.0f, kCenterDamp);
    currentY_ = lerp(currentY_, 0.0f, kCenterDamp * 0.9f);
  }

  switch (phase_) {
    case Phase::ToOvershoot:
    case Phase::ToSettle: {
      const unsigned long elapsed = nowMs - phaseStartMs_;
      const float t = (phaseDurationMs_ == 0)
                          ? 1.0f
                          : MathUtils::clamp(static_cast<float>(elapsed) / static_cast<float>(phaseDurationMs_), 0.0f, 1.0f);
      const float eased = (phase_ == Phase::ToOvershoot) ? easeOutCubic(t) : easeInOutCubic(t);
      currentX_ = lerp(phaseFromX_, phaseToX_, eased);
      currentY_ = lerp(phaseFromY_, phaseToY_, eased);

      if (elapsed >= phaseDurationMs_) {
        completePrimaryMotion(nowMs);
      }
      break;
    }

    case Phase::Fixation: {
      currentX_ = settleToX_;
      currentY_ = settleToY_;

      if (nowMs >= fixationUntilMs_) {
        if (pendingReturnToCenter_) {
          pendingReturnToCenter_ = false;
          startReturnToCenter(nowMs);
          break;
        }
        if (!hasExternalTarget_) {
          phase_ = Phase::Idle;
          nextIdleScanAtMs_ = nowMs + HardwareConfig::FaceGaze::IDLE_SCAN_INTERVAL_MS;
          break;
        }
      }

      const bool listeningContext = expression_ == ExpressionType::Listening;
      const bool thinkingContext = expression_ == ExpressionType::Thinking;
      const bool allowMicro = listeningContext || thinkingContext;
      if (allowMicro && nowMs >= nextMicroAtMs_) {
        startMicroSaccade(nowMs);
      }
      break;
    }

    case Phase::Micro: {
      const unsigned long elapsed = nowMs - phaseStartMs_;
      const float t = (phaseDurationMs_ == 0)
                          ? 1.0f
                          : MathUtils::clamp(static_cast<float>(elapsed) / static_cast<float>(phaseDurationMs_), 0.0f, 1.0f);
      const float pulse = sinf(3.1415926f * t);

      currentX_ = lerp(microBaseX_, microTargetX_, pulse);
      currentY_ = lerp(microBaseY_, microTargetY_, pulse);

      if (elapsed >= phaseDurationMs_) {
        phase_ = Phase::Fixation;
        phaseStartMs_ = nowMs;
        currentX_ = microBaseX_;
        currentY_ = microBaseY_;
        nextMicroAtMs_ = nowMs + microIntervalMs();
      }
      break;
    }

    case Phase::Idle:
    default:
      break;
  }

  output_.enabled = (fabsf(currentX_) > 0.004f) || (fabsf(currentY_) > 0.004f) || phase_ != Phase::Idle;
  output_.currentXNorm = clampX(currentX_);
  output_.currentYNorm = clampY(currentY_);
  output_.targetXNorm = clampX(settleToX_);
  output_.targetYNorm = clampY(settleToY_);
  output_.amplitude = MathUtils::clamp(sqrtf((output_.currentXNorm * output_.currentXNorm) +
                                             (output_.currentYNorm * output_.currentYNorm)),
                                       0.0f,
                                       1.0f);
  output_.fixationUntilMs = fixationUntilMs_;

  switch (phase_) {
    case Phase::ToOvershoot:
    case Phase::ToSettle:
      output_.mode = FaceGazeMode::Saccade;
      break;
    case Phase::Micro:
      output_.mode = FaceGazeMode::MicroSaccade;
      break;
    case Phase::Fixation:
      if (activeBehavior_ == FaceGazeBehavior::Scan) {
        output_.mode = FaceGazeMode::Scan;
      } else if (activeBehavior_ == FaceGazeBehavior::RecoveryGlance) {
        output_.mode = FaceGazeMode::Recover;
      } else {
        output_.mode = FaceGazeMode::Hold;
      }
      break;
    case Phase::Idle:
    default:
      output_.mode = output_.enabled ? FaceGazeMode::Hold : FaceGazeMode::None;
      break;
  }
}

const FaceGazeController::Output& FaceGazeController::output() const {
  return output_;
}

void FaceGazeController::queueTarget(float xNorm,
                                     float yNorm,
                                     FaceSaccadeSize saccadeSize,
                                     FaceGazeBehavior behavior,
                                     unsigned long fixationMs,
                                     bool external,
                                     unsigned long nowMs) {
  const float clampedX = clampX(xNorm);
  const float clampedY = clampY(yNorm);

  const float dx = clampedX - currentX_;
  const float dy = clampedY - currentY_;
  const float distance = sqrtf((dx * dx) + (dy * dy));

  if (distance < kDeadzone) {
    settleToX_ = clampedX;
    settleToY_ = clampedY;
    currentX_ = clampedX;
    currentY_ = clampedY;
    phase_ = Phase::Fixation;
    phaseStartMs_ = nowMs;
    fixationUntilMs_ = nowMs + ((fixationMs > 0) ? fixationMs : HardwareConfig::FaceGaze::FIXATION_SHORT_MS);
    nextMicroAtMs_ = nowMs + microIntervalMs();
    activeBehavior_ = behavior;
    pendingReturnToCenter_ = (behavior == FaceGazeBehavior::EdgePeek || behavior == FaceGazeBehavior::RecoveryGlance);
    hasExternalTarget_ = external;
    return;
  }

  const Durations durations = durationsFor(saccadeSize, distance);
  const float invDist = 1.0f / distance;
  const float dirX = dx * invDist;
  const float dirY = dy * invDist;
  const float overshootStep = MathUtils::clamp((distance * durations.overshoot) + 0.015f, 0.015f, 0.14f);

  phaseFromX_ = currentX_;
  phaseFromY_ = currentY_;
  phaseToX_ = clampX(clampedX + (dirX * overshootStep));
  phaseToY_ = clampY(clampedY + (dirY * overshootStep));
  settleToX_ = clampedX;
  settleToY_ = clampedY;

  phase_ = Phase::ToOvershoot;
  phaseStartMs_ = nowMs;
  phaseDurationMs_ = durations.saccadeMs;
  fixationUntilMs_ = nowMs + ((fixationMs > 0) ? fixationMs : durations.fixationMs);

  activeBehavior_ = behavior;
  hasExternalTarget_ = external;
  pendingReturnToCenter_ = (behavior == FaceGazeBehavior::EdgePeek || behavior == FaceGazeBehavior::RecoveryGlance || behavior == FaceGazeBehavior::Scan);
}

void FaceGazeController::startIdleScan(unsigned long nowMs) {
  const float amplitudeX = HardwareConfig::FaceGaze::IDLE_SCAN_X_NORM * (0.80f + (contextCuriosity_ * 0.35f));
  const float amplitudeY = HardwareConfig::FaceGaze::IDLE_SCAN_Y_NORM * (0.70f + (contextAttention_ * 0.30f));

  const float x = scanSignPositive_ ? amplitudeX : -amplitudeX;
  const float y = scanSignPositive_ ? amplitudeY : -amplitudeY;
  scanSignPositive_ = !scanSignPositive_;

  queueTarget(x,
              y,
              FaceSaccadeSize::Medium,
              FaceGazeBehavior::Scan,
              HardwareConfig::FaceGaze::FIXATION_SHORT_MS,
              false,
              nowMs);

  nextIdleScanAtMs_ = nowMs + HardwareConfig::FaceGaze::IDLE_SCAN_INTERVAL_MS;
}

void FaceGazeController::startMicroSaccade(unsigned long nowMs) {
  microBaseX_ = settleToX_;
  microBaseY_ = settleToY_;

  const float amp = microAmplitude();
  const float xDir = microSignPositive_ ? 1.0f : -1.0f;
  const float yDir = microSignPositive_ ? -0.5f : 0.5f;
  microSignPositive_ = !microSignPositive_;

  microTargetX_ = clampX(microBaseX_ + (amp * xDir));
  microTargetY_ = clampY(microBaseY_ + ((amp * 0.65f) * yDir));

  phase_ = Phase::Micro;
  phaseStartMs_ = nowMs;
  phaseDurationMs_ = HardwareConfig::FaceGaze::MICRO_DURATION_MS;
}

void FaceGazeController::startReturnToCenter(unsigned long nowMs) {
  queueTarget(0.0f,
              0.0f,
              FaceSaccadeSize::Short,
              FaceGazeBehavior::Hold,
              HardwareConfig::FaceGaze::FIXATION_SHORT_MS,
              false,
              nowMs);
}

void FaceGazeController::completePrimaryMotion(unsigned long nowMs) {
  if (phase_ == Phase::ToOvershoot) {
    phase_ = Phase::ToSettle;
    phaseFromX_ = phaseToX_;
    phaseFromY_ = phaseToY_;
    phaseToX_ = settleToX_;
    phaseToY_ = settleToY_;

    const float dx = settleToX_ - phaseFromX_;
    const float dy = settleToY_ - phaseFromY_;
    const float settleDist = sqrtf((dx * dx) + (dy * dy));
    const Durations settleDur = durationsFor(FaceSaccadeSize::Auto, settleDist);
    phaseDurationMs_ = settleDur.settleMs;
    phaseStartMs_ = nowMs;
    return;
  }

  phase_ = Phase::Fixation;
  phaseStartMs_ = nowMs;
  currentX_ = settleToX_;
  currentY_ = settleToY_;
  nextMicroAtMs_ = nowMs + microIntervalMs();
}

FaceGazeController::Durations FaceGazeController::durationsFor(FaceSaccadeSize size, float distance) const {
  const FaceSaccadeSize resolved = resolvedSaccadeSize(size, distance);

  Durations out;
  switch (resolved) {
    case FaceSaccadeSize::Short:
      out.saccadeMs = HardwareConfig::FaceGaze::SACCADE_SHORT_MS;
      out.settleMs = HardwareConfig::FaceGaze::SETTLE_SHORT_MS;
      out.fixationMs = HardwareConfig::FaceGaze::FIXATION_SHORT_MS;
      out.overshoot = HardwareConfig::FaceGaze::OVERSHOOT_SHORT;
      break;
    case FaceSaccadeSize::Long:
      out.saccadeMs = HardwareConfig::FaceGaze::SACCADE_LONG_MS;
      out.settleMs = HardwareConfig::FaceGaze::SETTLE_LONG_MS;
      out.fixationMs = HardwareConfig::FaceGaze::FIXATION_LONG_MS;
      out.overshoot = HardwareConfig::FaceGaze::OVERSHOOT_LONG;
      break;
    case FaceSaccadeSize::Medium:
    case FaceSaccadeSize::Auto:
    default:
      out.saccadeMs = HardwareConfig::FaceGaze::SACCADE_MEDIUM_MS;
      out.settleMs = HardwareConfig::FaceGaze::SETTLE_MEDIUM_MS;
      out.fixationMs = HardwareConfig::FaceGaze::FIXATION_MEDIUM_MS;
      out.overshoot = HardwareConfig::FaceGaze::OVERSHOOT_MEDIUM;
      break;
  }

  return out;
}

FaceSaccadeSize FaceGazeController::resolvedSaccadeSize(FaceSaccadeSize preferred, float distance) const {
  if (preferred != FaceSaccadeSize::Auto) {
    return preferred;
  }

  if (distance < 0.28f) {
    return FaceSaccadeSize::Short;
  }
  if (distance > 0.62f) {
    return FaceSaccadeSize::Long;
  }
  return FaceSaccadeSize::Medium;
}

unsigned long FaceGazeController::microIntervalMs() const {
  if (expression_ == ExpressionType::Listening) {
    return HardwareConfig::FaceGaze::MICRO_INTERVAL_LISTENING_MS;
  }
  if (expression_ == ExpressionType::Thinking) {
    return HardwareConfig::FaceGaze::MICRO_INTERVAL_THINKING_MS;
  }
  return HardwareConfig::FaceGaze::MICRO_INTERVAL_DEFAULT_MS;
}

float FaceGazeController::microAmplitude() const {
  if (expression_ == ExpressionType::Listening) {
    return HardwareConfig::FaceGaze::MICRO_AMPLITUDE_LISTENING;
  }
  if (expression_ == ExpressionType::Thinking) {
    return HardwareConfig::FaceGaze::MICRO_AMPLITUDE_THINKING;
  }
  return HardwareConfig::FaceGaze::MICRO_AMPLITUDE_DEFAULT;
}

float FaceGazeController::clampX(float v) {
  return MathUtils::clamp(v, -HardwareConfig::FaceGaze::MAX_X_NORM, HardwareConfig::FaceGaze::MAX_X_NORM);
}

float FaceGazeController::clampY(float v) {
  return MathUtils::clamp(v, -HardwareConfig::FaceGaze::MAX_Y_NORM, HardwareConfig::FaceGaze::MAX_Y_NORM);
}

float FaceGazeController::lerp(float a, float b, float t) {
  return a + ((b - a) * t);
}

float FaceGazeController::easeOutCubic(float t) {
  const float u = 1.0f - t;
  return 1.0f - (u * u * u);
}

float FaceGazeController::easeInOutCubic(float t) {
  if (t < 0.5f) {
    return 4.0f * t * t * t;
  }
  const float k = (-2.0f * t) + 2.0f;
  return 1.0f - ((k * k * k) * 0.5f);
}


