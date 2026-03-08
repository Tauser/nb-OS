#include "face_service.h"

#include "../../config/hardware_config.h"
#include "../../utils/math_utils.h"
#include <Arduino.h>
#include <math.h>

namespace {
#ifndef NCOS_SIM_MODE
#define NCOS_SIM_MODE 0
#endif

constexpr unsigned long kPerfWindowMs = 5000;
constexpr unsigned long kEmotionOutputIntervalMs = HardwareConfig::EmotionOutput::FACE_APPLY_INTERVAL_MS;

struct BlinkFrame {
  unsigned long tMs;
  float upper;
  float lower;
};

constexpr BlinkFrame kBlink[] = {
  {0, 0.12f, 0.05f},
  {60, 0.70f, 0.20f},
  {95, 0.92f, 0.35f},
  {140, 0.35f, 0.12f},
  {180, 0.12f, 0.05f}
};

EmotionPreset makePreset(float tilt, float squash, float stretch, float up, float low) {
  EmotionPreset p;
  p.tiltDeg = tilt;
  p.squashY = squash;
  p.stretchX = stretch;
  p.upperLid = up;
  p.lowerLid = low;
  return p;
}

EmotionPreset presetFor(ExpressionType e, bool left) {
  switch (e) {
    case ExpressionType::Neutral:
      return makePreset(0.0f, 0.98f, 1.04f, 0.14f, 0.06f);
    case ExpressionType::Curiosity:
      return makePreset(left ? -16.0f : 16.0f, 1.00f, 1.16f, 0.08f, 0.02f);
    case ExpressionType::FaceRecognized:
      return makePreset(left ? -10.0f : 10.0f, 1.06f, 1.10f, 0.10f, 0.04f);
    case ExpressionType::BatteryAlert:
      return makePreset(0.0f, 0.72f, 0.96f, 0.50f, 0.34f);
    case ExpressionType::Angry:
      return makePreset(left ? 20.0f : -20.0f, 0.84f, 1.20f, 0.48f, 0.02f);
    case ExpressionType::Sad:
      return makePreset(left ? -18.0f : 18.0f, 0.86f, 0.94f, 0.40f, 0.24f);
    case ExpressionType::Blink:
      return makePreset(0.0f, 0.10f, 1.02f, 0.48f, 0.48f);
    default:
      return makePreset(0.0f, 0.98f, 1.04f, 0.14f, 0.06f);
  }
}

#if NCOS_SIM_MODE
EmotionPreset boostPresetForSim(const EmotionPreset& inPreset) {
  EmotionPreset p = inPreset;
  p.tiltDeg *= 1.6f;
  p.squashY = MathUtils::clamp(1.0f + ((p.squashY - 1.0f) * 1.8f), 0.70f, 1.25f);
  p.stretchX = MathUtils::clamp(1.0f + ((p.stretchX - 1.0f) * 1.8f), 0.78f, 1.28f);
  p.upperLid = MathUtils::clamp(0.18f + ((p.upperLid - 0.18f) * 1.9f), 0.02f, 0.55f);
  p.lowerLid = MathUtils::clamp(0.10f + ((p.lowerLid - 0.10f) * 1.9f), 0.02f, 0.55f);
  return p;
}
#endif

float lerpF(float a, float b, float t) {
  return a + ((b - a) * t);
}

#if NCOS_SIM_MODE
const char* expressionName(ExpressionType e) {
  switch (e) {
    case ExpressionType::Neutral: return "neutral";
    case ExpressionType::Curiosity: return "curious";
    case ExpressionType::FaceRecognized: return "happy";
    case ExpressionType::BatteryAlert: return "alert";
    case ExpressionType::Angry: return "angry";
    case ExpressionType::Sad: return "sad";
    case ExpressionType::Blink: return "blink";
    default: return "unknown";
  }
}
#endif
}

FaceService::FaceService(IDisplayPort& displayPort, const IEmotionProvider& emotionProvider)
  : displayPort_(displayPort), emotionProvider_(emotionProvider), renderer_(displayPort) {
}

void FaceService::init() {
  randomSeed(millis());

  leftEye_.centerX = HardwareConfig::Face::LEFT_EYE_X;
  leftEye_.centerY = HardwareConfig::Face::EYE_Y;
  leftEye_.eyeRadius = HardwareConfig::Face::EYE_RADIUS;

  rightEye_.centerX = HardwareConfig::Face::RIGHT_EYE_X;
  rightEye_.centerY = HardwareConfig::Face::EYE_Y;
  rightEye_.eyeRadius = HardwareConfig::Face::EYE_RADIUS;

  targetLeftEye_ = leftEye_;
  targetRightEye_ = rightEye_;

  displayPort_.init();

  const unsigned long now = millis();
  scheduleNextBlink(now);
  lastIdleMotionMs_ = now;
  lastSmoothMs_ = now;
  lastEmotionOutputMs_ = now;
  perf_.windowStartMs = now;

  applyStateToTargets();
  smoothEyes(now);
  renderer_.render(leftEye_, rightEye_);
}

void FaceService::requestExpression(ExpressionType expression, EyeAnimPriority priority, unsigned long holdMs) {
  const unsigned long now = millis();
  if (expressionHoldUntilMs_ > now) {
    const int activePriority = static_cast<int>(currentPriority_);
    const int pendingPriority = static_cast<int>(requestedPriority_);
    const int incomingPriority = static_cast<int>(priority);
    const int strongestActive = activePriority > pendingPriority ? activePriority : pendingPriority;
    if (incomingPriority < strongestActive) {
      return;
    }
  }

  requestedExpression_ = expression;
  requestedPriority_ = priority;
  expressionHoldUntilMs_ = holdMs > 0 ? (now + holdMs) : 0;
}

void FaceService::update(unsigned long nowMs) {
  applyEmotionOutput(nowMs);
  stepStateMachine(nowMs);
  applyStateToTargets();
  updateBlink(nowMs);
  updateIdleMotion(nowMs);
  smoothEyes(nowMs);

  const unsigned long t0 = micros();
  const bool rendered = renderer_.render(leftEye_, rightEye_);
  const unsigned long renderUs = micros() - t0;

  recordPerf(nowMs, rendered, renderUs);
}

void FaceService::applyEmotionOutput(unsigned long nowMs) {
  if (nowMs - lastEmotionOutputMs_ < kEmotionOutputIntervalMs) {
    return;
  }
  lastEmotionOutputMs_ = nowMs;

  const EmotionState& emo = emotionProvider_.getEmotionState();

  ExpressionType target = ExpressionType::Neutral;
  if (emo.energy < HardwareConfig::EmotionOutput::FACE_ENERGY_LOW) {
    target = ExpressionType::BatteryAlert;
  } else if (emo.valence < HardwareConfig::EmotionOutput::FACE_VALENCE_NEG) {
    target = ExpressionType::Sad;
  } else if (emo.curiosity > HardwareConfig::EmotionOutput::FACE_CURIOSITY_HIGH) {
    target = ExpressionType::Curiosity;
  } else if (emo.valence > HardwareConfig::EmotionOutput::FACE_VALENCE_POS) {
    target = ExpressionType::FaceRecognized;
  }

  requestExpression(target, EyeAnimPriority::Emotion, HardwareConfig::EmotionOutput::FACE_EXPRESSION_HOLD_MS);
}

void FaceService::stepStateMachine(unsigned long nowMs) {
  if (expressionHoldUntilMs_ > 0 && nowMs > expressionHoldUntilMs_) {
    requestedExpression_ = ExpressionType::Neutral;
    requestedPriority_ = EyeAnimPriority::Idle;
    currentExpression_ = ExpressionType::Neutral;
    currentPriority_ = EyeAnimPriority::Idle;
    expressionHoldUntilMs_ = 0;
  }

  if (!blinking_ && static_cast<int>(requestedPriority_) >= static_cast<int>(currentPriority_)) {
    const ExpressionType prevExpression = currentExpression_;
    currentExpression_ = requestedExpression_;
    currentPriority_ = requestedPriority_;
#if NCOS_SIM_MODE
    if (currentExpression_ != prevExpression) {
      Serial.print("[FACE_STATE] expr=");
      Serial.println(expressionName(currentExpression_));
    }
#endif
  }
}

void FaceService::scheduleNextBlink(unsigned long nowMs) {
  const unsigned long interval = random(
    HardwareConfig::Face::BLINK_MIN_INTERVAL_MS,
    HardwareConfig::Face::BLINK_MAX_INTERVAL_MS
  );
  nextBlinkAtMs_ = nowMs + interval;
}

void FaceService::applyStateToTargets() {
  EmotionPreset l = presetFor(currentExpression_, true);
  EmotionPreset r = presetFor(currentExpression_, false);

#if NCOS_SIM_MODE
  if (currentExpression_ != ExpressionType::Neutral) {
    l = boostPresetForSim(l);
    r = boostPresetForSim(r);
  }
#endif

  targetLeftEye_.expression = currentExpression_;
  targetRightEye_.expression = currentExpression_;

  targetLeftEye_.tiltDeg = l.tiltDeg;
  targetRightEye_.tiltDeg = r.tiltDeg;

  targetLeftEye_.squashY = l.squashY;
  targetRightEye_.squashY = r.squashY;

  targetLeftEye_.stretchX = l.stretchX;
  targetRightEye_.stretchX = r.stretchX;

  targetLeftEye_.upperLid = l.upperLid;
  targetRightEye_.upperLid = r.upperLid;
  targetLeftEye_.lowerLid = l.lowerLid;
  targetRightEye_.lowerLid = r.lowerLid;

  targetLeftEye_.openness = baseOpenness_;
  targetRightEye_.openness = baseOpenness_;
}

void FaceService::updateBlink(unsigned long nowMs) {
  if (!blinking_ && nowMs >= nextBlinkAtMs_) {
    blinking_ = true;
    blinkStartMs_ = nowMs;
  }

  if (!blinking_) {
    return;
  }

  const unsigned long elapsed = nowMs - blinkStartMs_;
  const size_t last = (sizeof(kBlink) / sizeof(kBlink[0])) - 1;

  if (elapsed >= kBlink[last].tMs) {
    blinking_ = false;
    scheduleNextBlink(nowMs);
    return;
  }

  for (size_t i = 0; i < last; ++i) {
    const BlinkFrame& a = kBlink[i];
    const BlinkFrame& b = kBlink[i + 1];

    if (elapsed < a.tMs || elapsed > b.tMs) {
      continue;
    }

    const float t = static_cast<float>(elapsed - a.tMs) / static_cast<float>(b.tMs - a.tMs);
    const float e = easeInOutCubic(t);
    const float up = lerpF(a.upper, b.upper, e);
    const float low = lerpF(a.lower, b.lower, e);

    targetLeftEye_.upperLid = up;
    targetRightEye_.upperLid = up;
    targetLeftEye_.lowerLid = low;
    targetRightEye_.lowerLid = low;
    break;
  }
}

void FaceService::updateIdleMotion(unsigned long nowMs) {
  if (nowMs - lastIdleMotionMs_ < HardwareConfig::Face::LOOK_INTERVAL_MS) {
    return;
  }

  lastIdleMotionMs_ = nowMs;

  const EmotionState& emo = emotionProvider_.getEmotionState();
  const float activity = MathUtils::clamp((emo.arousal * HardwareConfig::EmotionOutput::FACE_ACTIVITY_AROUSAL_WEIGHT) + (emo.energy * HardwareConfig::EmotionOutput::FACE_ACTIVITY_ENERGY_WEIGHT), 0.0f, 1.0f);

  if (currentExpression_ == ExpressionType::FaceRecognized) {
    baseOpenness_ = random(102, 111) / 100.0f;
  } else {
    baseOpenness_ = random(95, 103) / 100.0f;
  }

  baseOpenness_ += (activity - 0.5f) * HardwareConfig::EmotionOutput::FACE_ACTIVITY_GAIN;
  baseOpenness_ -= (1.0f - emo.energy) * HardwareConfig::EmotionOutput::FACE_LOW_ENERGY_GAIN;
  baseOpenness_ = MathUtils::clamp(baseOpenness_, HardwareConfig::EmotionOutput::FACE_OPENNESS_MIN, HardwareConfig::EmotionOutput::FACE_OPENNESS_MAX);
}

void FaceService::smoothEyes(unsigned long nowMs) {
  const unsigned long dtMs = nowMs - lastSmoothMs_;
  lastSmoothMs_ = nowMs;

  const float dt = static_cast<float>(dtMs) / 1000.0f;
  // 14 Hz critically damped-ish response.
  float alpha = 1.0f - expf(-14.0f * dt);
#if NCOS_SIM_MODE
  alpha = MathUtils::clamp(alpha, 0.06f, 0.32f);
#else
  alpha = MathUtils::clamp(alpha, 0.08f, 0.45f);
#endif

  leftEye_.openness = lerpF(leftEye_.openness, targetLeftEye_.openness, alpha);
  rightEye_.openness = lerpF(rightEye_.openness, targetRightEye_.openness, alpha);

  leftEye_.upperLid = lerpF(leftEye_.upperLid, targetLeftEye_.upperLid, alpha);
  rightEye_.upperLid = lerpF(rightEye_.upperLid, targetRightEye_.upperLid, alpha);

  leftEye_.lowerLid = lerpF(leftEye_.lowerLid, targetLeftEye_.lowerLid, alpha);
  rightEye_.lowerLid = lerpF(rightEye_.lowerLid, targetRightEye_.lowerLid, alpha);

  leftEye_.tiltDeg = lerpF(leftEye_.tiltDeg, targetLeftEye_.tiltDeg, alpha);
  rightEye_.tiltDeg = lerpF(rightEye_.tiltDeg, targetRightEye_.tiltDeg, alpha);

  leftEye_.squashY = lerpF(leftEye_.squashY, targetLeftEye_.squashY, alpha);
  rightEye_.squashY = lerpF(rightEye_.squashY, targetRightEye_.squashY, alpha);

  leftEye_.stretchX = lerpF(leftEye_.stretchX, targetLeftEye_.stretchX, alpha);
  rightEye_.stretchX = lerpF(rightEye_.stretchX, targetRightEye_.stretchX, alpha);

  leftEye_.expression = targetLeftEye_.expression;
  rightEye_.expression = targetRightEye_.expression;
}

float FaceService::easeInOutCubic(float t) const {
  if (t < 0.5f) {
    return 4.0f * t * t * t;
  }
  const float k = (-2.0f * t) + 2.0f;
  return 1.0f - ((k * k * k) / 2.0f);
}

void FaceService::recordPerf(unsigned long nowMs, bool rendered, unsigned long renderUs) {
  perf_.updates++;

  if (rendered) {
    perf_.renderedFrames++;
    perf_.avgRenderUsAccum += renderUs;
    if (renderUs > perf_.maxRenderUs) {
      perf_.maxRenderUs = renderUs;
    }
  } else {
    perf_.droppedFrames++;
  }

  if (nowMs - perf_.windowStartMs < kPerfWindowMs) {
    return;
  }

#if 0
  Serial.print("[FACE_PERF] updates=");
  Serial.print(perf_.updates);
  Serial.print(" rendered=");
  Serial.print(perf_.renderedFrames);
  Serial.print(" skipped=");
  Serial.print(perf_.droppedFrames);
  Serial.print(" avg_us=");
  if (perf_.renderedFrames > 0) {
    Serial.print(perf_.avgRenderUsAccum / perf_.renderedFrames);
  } else {
    Serial.print(0);
  }
  Serial.print(" max_us=");
  Serial.println(perf_.maxRenderUs);
#endif

  perf_.windowStartMs = nowMs;
  perf_.updates = 0;
  perf_.renderedFrames = 0;
  perf_.droppedFrames = 0;
  perf_.maxRenderUs = 0;
  perf_.avgRenderUsAccum = 0;
}







