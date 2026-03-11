#pragma once

#include "../../models/emotion_state.h"
#include "../../models/expression_types.h"
#include <stddef.h>
#include <stdint.h>

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

enum class FacePresetReadabilityTier : uint8_t {
  TierA = 0,
  TierB,
  TierC
};

enum class FacePresetUsageRole : uint8_t {
  Primary = 0,
  Situational
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

// Layer blend policy keeps presets stable when combined with compositor layers.
struct PresetLayerBlendProfile {
  float moodGain = 1.0f;
  float attentionGain = 1.0f;
  float gazeGain = 1.0f;
  float idleGain = 1.0f;
  float transientGain = 1.0f;
  float clipGain = 1.0f;
  float blinkGain = 1.0f;
};

// Tuning guardrails are preset-specific limits used by runtime tuner.
struct PresetTuningGuardrails {
  float widthMin = 0.75f;
  float widthMax = 1.55f;
  float heightMin = 0.50f;
  float heightMax = 1.30f;
  float roundnessMin = 0.20f;
  float roundnessMax = 0.90f;
  float opennessMin = 0.35f;
  float opennessMax = 1.15f;
  float upperLidMin = 0.0f;
  float upperLidMax = 0.95f;
  float lowerLidMin = 0.0f;
  float lowerLidMax = 0.90f;
};

struct PresetUsageProfile {
  FacePresetUsageRole role = FacePresetUsageRole::Primary;
  unsigned long recommendedHoldMs = 1000;
  bool prefersAttentionLock = false;
  bool expectsClipSupport = true;
  const char* tuningNotes = "general";
};

struct ExpressionPreset {
  FacePresetId id = FacePresetId::Neutral;
  const char* name = "neutral_premium";

  FacePresetReadabilityTier readabilityTier = FacePresetReadabilityTier::TierC;
  // Kept for migration visibility. F4B official library sets this to false.
  bool explorationOnly = false;

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
  PresetLayerBlendProfile layerBlend{};
  PresetTuningGuardrails tuning{};
  PresetUsageProfile usage{};
};

const ExpressionPreset& getExpressionPreset(FacePresetId id);
const ExpressionPreset* facePresetLibrary(size_t* outCount);
bool validateFacePresetLibrary(const char** outReason = nullptr);

bool parseFacePresetId(const char* name, FacePresetId* outId);
const char* facePresetName(FacePresetId id);
const char* facePresetTierName(FacePresetReadabilityTier tier);
bool facePresetLibraryFrozen();

const PresetLayerBlendProfile& facePresetLayerBlend(FacePresetId id);
const PresetTuningGuardrails& facePresetTuningGuardrails(FacePresetId id);
const PresetUsageProfile& facePresetUsageProfile(FacePresetId id);

FacePresetId choosePreset(ExpressionType expression, const EmotionState& emotion);
