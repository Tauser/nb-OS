#include "expression_preset.h"

#include <string.h>

namespace {
PresetLayerBlendProfile defaultLayerBlend(FacePresetReadabilityTier tier) {
  PresetLayerBlendProfile blend;
  switch (tier) {
    case FacePresetReadabilityTier::TierA:
      blend.moodGain = 0.74f;
      blend.attentionGain = 0.76f;
      blend.gazeGain = 0.86f;
      blend.idleGain = 0.68f;
      blend.transientGain = 0.82f;
      blend.clipGain = 0.92f;
      blend.blinkGain = 0.84f;
      break;
    case FacePresetReadabilityTier::TierB:
      blend.moodGain = 0.86f;
      blend.attentionGain = 0.88f;
      blend.gazeGain = 0.94f;
      blend.idleGain = 0.82f;
      blend.transientGain = 0.90f;
      blend.clipGain = 0.96f;
      blend.blinkGain = 0.92f;
      break;
    case FacePresetReadabilityTier::TierC:
    default:
      blend.moodGain = 1.00f;
      blend.attentionGain = 1.00f;
      blend.gazeGain = 1.00f;
      blend.idleGain = 0.92f;
      blend.transientGain = 0.96f;
      blend.clipGain = 1.00f;
      blend.blinkGain = 1.00f;
      break;
  }
  return blend;
}

PresetTuningGuardrails defaultTuning(FacePresetId id) {
  PresetTuningGuardrails tuning;

  switch (id) {
    case FacePresetId::LowEnergy:
      tuning.widthMin = 0.92f;
      tuning.widthMax = 1.18f;
      tuning.heightMin = 0.58f;
      tuning.heightMax = 0.92f;
      tuning.roundnessMin = 0.48f;
      tuning.roundnessMax = 0.82f;
      tuning.opennessMin = 0.56f;
      tuning.opennessMax = 0.92f;
      tuning.upperLidMin = 0.20f;
      tuning.upperLidMax = 0.88f;
      tuning.lowerLidMin = 0.10f;
      tuning.lowerLidMax = 0.62f;
      return tuning;

    case FacePresetId::Surprised:
      tuning.widthMin = 1.06f;
      tuning.widthMax = 1.34f;
      tuning.heightMin = 0.96f;
      tuning.heightMax = 1.24f;
      tuning.roundnessMin = 0.32f;
      tuning.roundnessMax = 0.64f;
      tuning.opennessMin = 0.98f;
      tuning.opennessMax = 1.20f;
      tuning.upperLidMin = 0.0f;
      tuning.upperLidMax = 0.16f;
      tuning.lowerLidMin = 0.0f;
      tuning.lowerLidMax = 0.18f;
      return tuning;

    case FacePresetId::Sleepy:
      tuning.widthMin = 0.92f;
      tuning.widthMax = 1.18f;
      tuning.heightMin = 0.62f;
      tuning.heightMax = 0.90f;
      tuning.roundnessMin = 0.50f;
      tuning.roundnessMax = 0.78f;
      tuning.opennessMin = 0.66f;
      tuning.opennessMax = 0.96f;
      tuning.upperLidMin = 0.18f;
      tuning.upperLidMax = 0.76f;
      tuning.lowerLidMin = 0.08f;
      tuning.lowerLidMax = 0.46f;
      return tuning;

    case FacePresetId::Listening:
    case FacePresetId::Focused:
      tuning.widthMin = 1.00f;
      tuning.widthMax = 1.28f;
      tuning.heightMin = 0.78f;
      tuning.heightMax = 1.02f;
      tuning.roundnessMin = 0.30f;
      tuning.roundnessMax = 0.60f;
      tuning.opennessMin = 0.86f;
      tuning.opennessMax = 1.08f;
      tuning.upperLidMin = 0.02f;
      tuning.upperLidMax = 0.42f;
      tuning.lowerLidMin = 0.0f;
      tuning.lowerLidMax = 0.24f;
      return tuning;

    default:
      return tuning;
  }
}

PresetUsageProfile defaultUsage(FacePresetId id, unsigned long holdMs) {
  PresetUsageProfile usage;
  usage.recommendedHoldMs = holdMs;

  switch (id) {
    case FacePresetId::Surprised:
    case FacePresetId::LowEnergy:
    case FacePresetId::Listening:
    case FacePresetId::Thinking:
    case FacePresetId::Shy:
      usage.role = FacePresetUsageRole::Situational;
      break;
    default:
      usage.role = FacePresetUsageRole::Primary;
      break;
  }

  usage.prefersAttentionLock =
      (id == FacePresetId::Listening) || (id == FacePresetId::Focused) || (id == FacePresetId::Thinking);
  usage.expectsClipSupport = true;

  switch (id) {
    case FacePresetId::Neutral: usage.tuningNotes = "baseline face"; break;
    case FacePresetId::Curious: usage.tuningNotes = "curiosity without nervousness"; break;
    case FacePresetId::Sleepy: usage.tuningNotes = "slow and low-energy readability"; break;
    case FacePresetId::HappySoft: usage.tuningNotes = "warm social acknowledgement"; break;
    case FacePresetId::Focused: usage.tuningNotes = "high attention with stable silhouette"; break;
    case FacePresetId::Shy: usage.tuningNotes = "retreative but friendly"; break;
    case FacePresetId::Surprised: usage.tuningNotes = "fast high-contrast reaction"; break;
    case FacePresetId::LowEnergy: usage.tuningNotes = "battery/energy constrained state"; break;
    case FacePresetId::Listening: usage.tuningNotes = "voice-attention lock"; break;
    case FacePresetId::Thinking: usage.tuningNotes = "cognitive idle loop"; break;
    default: usage.tuningNotes = "general"; break;
  }

  return usage;
}

ExpressionPreset makePreset(FacePresetId id,
                            const char* name,
                            FacePresetReadabilityTier tier,
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
  p.readabilityTier = tier;
  p.explorationOnly = false;
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

  p.layerBlend = defaultLayerBlend(tier);
  p.tuning = defaultTuning(id);
  p.usage = defaultUsage(id, transitionHoldMs);
  return p;
}

const ExpressionPreset kPresets[] = {
    makePreset(FacePresetId::Neutral, "neutral_premium", FacePresetReadabilityTier::TierC,
               1.12f, 0.96f, 0.58f, 0.0f, 1.00f, 0.08f, 0.03f, 1.00f,
               3500, 6800, 190, 0.89f, 0.31f, 14.0f, 0.08f, 0.44f, 220, 950, 180, 1.00f),
    makePreset(FacePresetId::Curious, "curious_soft", FacePresetReadabilityTier::TierC,
               1.20f, 0.98f, 0.50f, 9.0f, 1.03f, 0.05f, 0.02f, 1.03f,
               3200, 6000, 170, 0.84f, 0.27f, 16.0f, 0.10f, 0.50f, 240, 1100, 220, 0.92f),
    makePreset(FacePresetId::Sleepy, "sleepy_sink", FacePresetReadabilityTier::TierA,
               1.05f, 0.76f, 0.64f, -2.0f, 1.00f, 0.40f, 0.17f, 0.80f,
               5200, 9800, 260, 0.95f, 0.42f, 9.0f, 0.05f, 0.28f, 430, 2200, 360, 1.42f),
    makePreset(FacePresetId::HappySoft, "warm_happy", FacePresetReadabilityTier::TierC,
               1.14f, 1.06f, 0.60f, 6.0f, 1.00f, 0.06f, 0.02f, 1.06f,
               3000, 5400, 160, 0.84f, 0.25f, 15.0f, 0.10f, 0.48f, 230, 1200, 210, 0.94f),
    makePreset(FacePresetId::Focused, "focused_listen", FacePresetReadabilityTier::TierB,
               1.16f, 0.88f, 0.42f, 14.0f, 1.02f, 0.22f, 0.03f, 0.98f,
               2800, 5000, 150, 0.81f, 0.23f, 18.0f, 0.11f, 0.56f, 190, 1000, 180, 0.80f),
    makePreset(FacePresetId::Shy, "shy_peek", FacePresetReadabilityTier::TierB,
               1.06f, 0.86f, 0.66f, 12.0f, 0.99f, 0.28f, 0.12f, 0.90f,
               4300, 7600, 220, 0.90f, 0.34f, 11.0f, 0.07f, 0.34f, 320, 1600, 280, 1.18f),
    makePreset(FacePresetId::Surprised, "surprised_open", FacePresetReadabilityTier::TierA,
               1.23f, 1.16f, 0.48f, 0.0f, 1.04f, 0.01f, 0.00f, 1.12f,
               2600, 5000, 140, 0.78f, 0.20f, 19.0f, 0.12f, 0.60f, 160, 900, 140, 0.70f),
    makePreset(FacePresetId::LowEnergy, "low_energy_flat", FacePresetReadabilityTier::TierA,
               1.00f, 0.70f, 0.68f, -1.0f, 1.00f, 0.56f, 0.36f, 0.70f,
               6400, 11800, 330, 0.96f, 0.45f, 8.0f, 0.04f, 0.22f, 500, 2800, 420, 1.58f),
    makePreset(FacePresetId::Listening, "attention_lock", FacePresetReadabilityTier::TierA,
               1.10f, 0.92f, 0.46f, 2.0f, 1.02f, 0.18f, 0.02f, 0.99f,
               3000, 5400, 165, 0.82f, 0.24f, 17.0f, 0.10f, 0.52f, 210, 1000, 190, 0.86f),
    makePreset(FacePresetId::Thinking, "thinking_side", FacePresetReadabilityTier::TierB,
               1.13f, 0.92f, 0.50f, 16.0f, 1.00f, 0.16f, 0.05f, 0.96f,
               3600, 6800, 205, 0.88f, 0.30f, 13.0f, 0.08f, 0.42f, 270, 1400, 260, 1.10f),
};

const unsigned int kPresetCount = sizeof(kPresets) / sizeof(kPresets[0]);

bool isExpectedTier(FacePresetId id, FacePresetReadabilityTier tier) {
  switch (id) {
    case FacePresetId::Surprised:
    case FacePresetId::Sleepy:
    case FacePresetId::LowEnergy:
    case FacePresetId::Listening:
      return tier == FacePresetReadabilityTier::TierA;
    case FacePresetId::Focused:
    case FacePresetId::Shy:
    case FacePresetId::Thinking:
      return tier == FacePresetReadabilityTier::TierB;
    case FacePresetId::Neutral:
    case FacePresetId::Curious:
    case FacePresetId::HappySoft:
      return tier == FacePresetReadabilityTier::TierC;
    default:
      return false;
  }
}

const ExpressionPreset& fallbackPreset() {
  return kPresets[0];
}
} // namespace

const ExpressionPreset& getExpressionPreset(FacePresetId id) {
  const unsigned int index = static_cast<unsigned int>(id);
  if (index >= kPresetCount) {
    return fallbackPreset();
  }
  return kPresets[index];
}

const ExpressionPreset* facePresetLibrary(size_t* outCount) {
  if (outCount != nullptr) {
    *outCount = kPresetCount;
  }
  return kPresets;
}

bool validateFacePresetLibrary(const char** outReason) {
  if (outReason != nullptr) {
    *outReason = nullptr;
  }

  if (kPresetCount != 10U) {
    if (outReason != nullptr) {
      *outReason = "preset_count";
    }
    return false;
  }

  for (size_t i = 0; i < kPresetCount; ++i) {
    const ExpressionPreset& p = kPresets[i];

    if (p.name == nullptr || p.name[0] == '\0') {
      if (outReason != nullptr) {
        *outReason = "name_missing";
      }
      return false;
    }

    if (p.explorationOnly) {
      if (outReason != nullptr) {
        *outReason = "exploration_flag";
      }
      return false;
    }

    if (!isExpectedTier(p.id, p.readabilityTier)) {
      if (outReason != nullptr) {
        *outReason = "tier_mismatch";
      }
      return false;
    }

    if (p.tuning.widthMin >= p.tuning.widthMax ||
        p.tuning.heightMin >= p.tuning.heightMax ||
        p.tuning.roundnessMin >= p.tuning.roundnessMax ||
        p.tuning.opennessMin >= p.tuning.opennessMax ||
        p.tuning.upperLidMin > p.tuning.upperLidMax ||
        p.tuning.lowerLidMin > p.tuning.lowerLidMax) {
      if (outReason != nullptr) {
        *outReason = "invalid_tuning_guardrails";
      }
      return false;
    }

    if (p.widthScale < p.tuning.widthMin || p.widthScale > p.tuning.widthMax ||
        p.heightScale < p.tuning.heightMin || p.heightScale > p.tuning.heightMax ||
        p.roundness < p.tuning.roundnessMin || p.roundness > p.tuning.roundnessMax ||
        p.baseOpenness < p.tuning.opennessMin || p.baseOpenness > p.tuning.opennessMax ||
        p.upperLid < p.tuning.upperLidMin || p.upperLid > p.tuning.upperLidMax ||
        p.lowerLid < p.tuning.lowerLidMin || p.lowerLid > p.tuning.lowerLidMax) {
      if (outReason != nullptr) {
        *outReason = "base_outside_tuning";
      }
      return false;
    }
  }

  return true;
}

bool parseFacePresetId(const char* name, FacePresetId* outId) {
  if (name == nullptr || outId == nullptr) {
    return false;
  }

  if (strcmp(name, "neutral_premium") == 0) { *outId = FacePresetId::Neutral; return true; }
  if (strcmp(name, "curious_soft") == 0) { *outId = FacePresetId::Curious; return true; }
  if (strcmp(name, "sleepy_sink") == 0) { *outId = FacePresetId::Sleepy; return true; }
  if (strcmp(name, "warm_happy") == 0) { *outId = FacePresetId::HappySoft; return true; }
  if (strcmp(name, "focused_listen") == 0) { *outId = FacePresetId::Focused; return true; }
  if (strcmp(name, "shy_peek") == 0) { *outId = FacePresetId::Shy; return true; }
  if (strcmp(name, "surprised_open") == 0) { *outId = FacePresetId::Surprised; return true; }
  if (strcmp(name, "low_energy_flat") == 0) { *outId = FacePresetId::LowEnergy; return true; }
  if (strcmp(name, "attention_lock") == 0) { *outId = FacePresetId::Listening; return true; }
  if (strcmp(name, "thinking_side") == 0) { *outId = FacePresetId::Thinking; return true; }

  // Legacy aliases kept for transition.
  if (strcmp(name, "neutral") == 0) { *outId = FacePresetId::Neutral; return true; }
  if (strcmp(name, "curious") == 0) { *outId = FacePresetId::Curious; return true; }
  if (strcmp(name, "sleepy") == 0) { *outId = FacePresetId::Sleepy; return true; }
  if (strcmp(name, "happy_soft") == 0 || strcmp(name, "happy") == 0) { *outId = FacePresetId::HappySoft; return true; }
  if (strcmp(name, "focused") == 0) { *outId = FacePresetId::Focused; return true; }
  if (strcmp(name, "shy") == 0) { *outId = FacePresetId::Shy; return true; }
  if (strcmp(name, "surprised") == 0) { *outId = FacePresetId::Surprised; return true; }
  if (strcmp(name, "low_energy") == 0 || strcmp(name, "alert") == 0) { *outId = FacePresetId::LowEnergy; return true; }
  if (strcmp(name, "listening") == 0) { *outId = FacePresetId::Listening; return true; }
  if (strcmp(name, "thinking") == 0) { *outId = FacePresetId::Thinking; return true; }

  return false;
}

const char* facePresetName(FacePresetId id) {
  return getExpressionPreset(id).name;
}

const char* facePresetTierName(FacePresetReadabilityTier tier) {
  switch (tier) {
    case FacePresetReadabilityTier::TierA: return "tier_a";
    case FacePresetReadabilityTier::TierB: return "tier_b";
    case FacePresetReadabilityTier::TierC: return "tier_c";
    default: return "tier_unknown";
  }
}

bool facePresetLibraryFrozen() {
  return true;
}

const PresetLayerBlendProfile& facePresetLayerBlend(FacePresetId id) {
  return getExpressionPreset(id).layerBlend;
}

const PresetTuningGuardrails& facePresetTuningGuardrails(FacePresetId id) {
  return getExpressionPreset(id).tuning;
}

const PresetUsageProfile& facePresetUsageProfile(FacePresetId id) {
  return getExpressionPreset(id).usage;
}

FacePresetId choosePreset(ExpressionType expression, const EmotionState& emotion) {
  if (expression == ExpressionType::Surprised) {
    return FacePresetId::Surprised;
  }
  if (expression == ExpressionType::Listening) {
    return FacePresetId::Listening;
  }
  if (expression == ExpressionType::Thinking) {
    return FacePresetId::Thinking;
  }
  if (expression == ExpressionType::Shy) {
    return FacePresetId::Shy;
  }
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

