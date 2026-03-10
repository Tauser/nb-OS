#pragma once

#include "../../models/emotion_state.h"
#include "../../models/expression_types.h"

enum class FacePresetId {
  Neutral = 0,
  Curious,
  Sleepy,
  HappySoft,
  Focused,
  Shy,
  Surprised,
  LowEnergy,
  Listening,
  Thinking
};

struct BlinkProfile {
  unsigned long minIntervalMs = 3200;
  unsigned long maxIntervalMs = 6200;
  unsigned long durationMs = 190;
  float upperPeak = 0.92f;
  float lowerPeak = 0.35f;
};

struct TransitionProfile {
  float responseHz = 14.0f;
  float alphaMin = 0.08f;
  float alphaMax = 0.45f;
  unsigned long blendMs = 260;
  unsigned long holdMs = 1200;
  unsigned long settleMs = 220;
  float blendCurveExp = 1.0f;
};

struct ExpressionPreset {
  FacePresetId id = FacePresetId::Neutral;
  const char* name = "neutral";

  // Core visual signature
  float widthScale = 1.0f;
  float heightScale = 1.0f;
  float roundness = 0.52f;
  float tiltDeg = 0.0f;
  float spacingScale = 1.0f;

  float upperLid = 0.08f;
  float lowerLid = 0.02f;
  float baseOpenness = 1.0f;

  BlinkProfile blink{};
  TransitionProfile transition{};
};

const ExpressionPreset& getExpressionPreset(FacePresetId id);
bool parseFacePresetId(const char* name, FacePresetId* outId);
const char* facePresetName(FacePresetId id);
FacePresetId choosePreset(ExpressionType expression, const EmotionState& emotion);
