#include "expression_preset.h"

namespace {
ExpressionPreset makePreset(FacePresetId id,
                            const char* name,
                            float widthScale,
                            float heightScale,
                            float roundness,
                            float tiltDeg,
                            float spacingScale,
                            float upperLid,
                            float lowerLid,
                            float baseOpenness,
                            unsigned long blinkMinMs,
                            unsigned long blinkMaxMs,
                            unsigned long blinkDurationMs,
                            float blinkUpperPeak,
                            float blinkLowerPeak,
                            float transitionResponseHz,
                            float transitionAlphaMin,
                            float transitionAlphaMax,
                            unsigned long transitionBlendMs,
                            unsigned long transitionHoldMs,
                            unsigned long transitionSettleMs,
                            float transitionBlendCurveExp) {
  ExpressionPreset p;
  p.id = id;
  p.name = name;
  p.widthScale = widthScale;
  p.heightScale = heightScale;
  p.roundness = roundness;
  p.tiltDeg = tiltDeg;
  p.spacingScale = spacingScale;
  p.upperLid = upperLid;
  p.lowerLid = lowerLid;
  p.baseOpenness = baseOpenness;

  p.blink.minIntervalMs = blinkMinMs;
  p.blink.maxIntervalMs = blinkMaxMs;
  p.blink.durationMs = blinkDurationMs;
  p.blink.upperPeak = blinkUpperPeak;
  p.blink.lowerPeak = blinkLowerPeak;

  p.transition.responseHz = transitionResponseHz;
  p.transition.alphaMin = transitionAlphaMin;
  p.transition.alphaMax = transitionAlphaMax;
  p.transition.blendMs = transitionBlendMs;
  p.transition.holdMs = transitionHoldMs;
  p.transition.settleMs = transitionSettleMs;
  p.transition.blendCurveExp = transitionBlendCurveExp;
  return p;
}

const ExpressionPreset kPresets[] = {
    makePreset(FacePresetId::Neutral, "neutral", 1.10f, 1.00f, 0.54f, 0.0f, 1.00f, 0.06f, 0.02f, 1.02f,
               3400, 6400, 190, 0.90f, 0.32f, 14.0f, 0.08f, 0.45f, 220, 900, 180, 1.00f),

    makePreset(FacePresetId::Curious, "curious", 1.24f, 0.96f, 0.48f, 24.0f, 1.04f, 0.04f, 0.01f, 1.03f,
               3200, 6000, 170, 0.86f, 0.28f, 16.0f, 0.10f, 0.50f, 260, 1300, 220, 0.90f),

    makePreset(FacePresetId::Sleepy, "sleepy", 1.06f, 0.80f, 0.60f, 0.0f, 1.00f, 0.34f, 0.16f, 0.86f,
               5000, 9000, 260, 0.94f, 0.40f, 10.0f, 0.05f, 0.28f, 420, 2200, 360, 1.40f),

    makePreset(FacePresetId::HappySoft, "happy_soft", 1.14f, 1.08f, 0.56f, 10.0f, 1.00f, 0.08f, 0.03f, 1.07f,
               3000, 5600, 170, 0.85f, 0.26f, 15.0f, 0.10f, 0.48f, 240, 1400, 220, 0.92f),

    makePreset(FacePresetId::Focused, "focused", 1.14f, 0.90f, 0.46f, 14.0f, 1.02f, 0.26f, 0.02f, 0.98f,
               2800, 5200, 150, 0.82f, 0.24f, 17.0f, 0.11f, 0.54f, 200, 1200, 200, 0.82f),

    makePreset(FacePresetId::Shy, "shy", 1.08f, 0.88f, 0.58f, 16.0f, 0.98f, 0.24f, 0.12f, 0.92f,
               4200, 7600, 220, 0.90f, 0.34f, 12.0f, 0.07f, 0.34f, 320, 1700, 280, 1.18f),

    makePreset(FacePresetId::Surprised, "surprised", 1.20f, 1.12f, 0.50f, 0.0f, 1.03f, 0.02f, 0.00f, 1.12f,
               2600, 5000, 140, 0.78f, 0.20f, 19.0f, 0.12f, 0.60f, 160, 900, 140, 0.70f),

    makePreset(FacePresetId::LowEnergy, "low_energy", 0.96f, 0.72f, 0.62f, 0.0f, 0.99f, 0.50f, 0.34f, 0.74f,
               6200, 11000, 320, 0.96f, 0.44f, 8.0f, 0.04f, 0.22f, 480, 2600, 400, 1.55f),

    makePreset(FacePresetId::Listening, "listening", 1.08f, 0.98f, 0.55f, 6.0f, 1.01f, 0.08f, 0.03f, 1.00f,
               3000, 5600, 170, 0.84f, 0.26f, 15.0f, 0.09f, 0.46f, 240, 1200, 220, 0.95f),

    makePreset(FacePresetId::Thinking, "thinking", 1.12f, 0.94f, 0.52f, 12.0f, 1.00f, 0.14f, 0.04f, 0.97f,
               3600, 6800, 200, 0.88f, 0.30f, 13.0f, 0.08f, 0.42f, 260, 1400, 260, 1.10f),
};

const unsigned int kPresetCount = sizeof(kPresets) / sizeof(kPresets[0]);

const ExpressionPreset& fallbackPreset() {
  return kPresets[0];
}
}

const ExpressionPreset& getExpressionPreset(FacePresetId id) {
  const unsigned int index = static_cast<unsigned int>(id);
  if (index >= kPresetCount) {
    return fallbackPreset();
  }
  return kPresets[index];
}

FacePresetId choosePreset(ExpressionType expression, const EmotionState& emotion) {
  if (expression == ExpressionType::BatteryAlert || emotion.energy < 0.26f) {
    return FacePresetId::LowEnergy;
  }

  if (expression == ExpressionType::Curiosity) {
    return FacePresetId::Curious;
  }

  if (expression == ExpressionType::FaceRecognized) {
    return FacePresetId::HappySoft;
  }

  if (expression == ExpressionType::Angry) {
    return FacePresetId::Focused;
  }

  if (expression == ExpressionType::Sad) {
    return FacePresetId::Sleepy;
  }

  if (emotion.arousal > 0.80f) {
    return FacePresetId::Surprised;
  }

  if (emotion.attention > 0.72f && emotion.curiosity < 0.42f) {
    return FacePresetId::Listening;
  }

  if (emotion.curiosity > 0.68f && emotion.attention > 0.52f) {
    return FacePresetId::Thinking;
  }

  if (emotion.valence < -0.25f && emotion.arousal < 0.45f) {
    return FacePresetId::Shy;
  }

  return FacePresetId::Neutral;
}
