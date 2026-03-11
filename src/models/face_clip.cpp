#include "face_clip.h"

#include "../utils/math_utils.h"

namespace {

FaceClipKeyframe kWakeUpFrames[] = {
    {0, 0, 2, -0.42f, 0.40f, 0.14f, -3.0f, -0.12f, -0.05f, 0.04f, 0.00f},
    {180, 0, 1, -0.18f, 0.18f, 0.08f, -1.4f, -0.06f, -0.02f, 0.02f, 0.00f},
    {420, 0, 0, 0.02f, -0.05f, -0.01f, 0.6f, 0.03f, 0.01f, 0.00f, 0.01f},
    {900, 0, 0, 0.00f, 0.00f, 0.00f, 0.0f, 0.00f, 0.00f, 0.00f, 0.00f},
};

FaceClipKeyframe kGoToSleepFrames[] = {
    {0, 0, 0, 0.00f, 0.00f, 0.00f, 0.0f, 0.00f, 0.00f, 0.00f, 0.00f},
    {340, 0, 1, -0.12f, 0.10f, 0.04f, -0.8f, -0.03f, -0.02f, 0.02f, 0.00f},
    {780, 0, 2, -0.28f, 0.24f, 0.08f, -1.6f, -0.08f, -0.04f, 0.04f, 0.00f},
    {1100, 0, 2, -0.40f, 0.36f, 0.13f, -2.2f, -0.11f, -0.06f, 0.05f, 0.00f},
};

FaceClipKeyframe kAttentionRecoveryFrames[] = {
    {0, -4, 0, 0.00f, -0.02f, -0.01f, -2.4f, 0.02f, 0.02f, -0.02f, 0.02f},
    {120, 5, 0, 0.02f, -0.03f, -0.01f, 2.8f, 0.03f, 0.02f, -0.03f, 0.02f},
    {260, -2, 0, 0.01f, -0.02f, -0.01f, -1.1f, 0.02f, 0.01f, -0.01f, 0.01f},
    {440, 1, 0, 0.00f, -0.01f, 0.00f, 0.8f, 0.01f, 0.01f, 0.00f, 0.00f},
    {620, 0, 0, 0.00f, 0.00f, 0.00f, 0.0f, 0.00f, 0.00f, 0.00f, 0.00f},
};

FaceClipKeyframe kThinkingLoopFrames[] = {
    {0, -3, 0, -0.04f, 0.06f, 0.02f, -1.8f, -0.03f, 0.02f, -0.02f, 0.01f},
    {260, -1, 0, -0.02f, 0.05f, 0.02f, -0.9f, -0.02f, 0.01f, -0.01f, 0.01f},
    {560, 2, 0, 0.00f, 0.03f, 0.01f, 1.2f, 0.01f, 0.01f, 0.00f, 0.00f},
    {930, 3, 0, -0.01f, 0.04f, 0.01f, 1.9f, 0.02f, 0.02f, -0.01f, 0.01f},
    {1200, 1, 0, -0.02f, 0.05f, 0.02f, 0.8f, -0.01f, 0.01f, -0.01f, 0.01f},
    {1400, 0, 0, 0.00f, 0.00f, 0.00f, 0.0f, 0.00f, 0.00f, 0.00f, 0.00f},
};

FaceClipKeyframe kSoftListenFrames[] = {
    {0, 2, 0, -0.03f, 0.04f, 0.01f, 1.6f, -0.02f, 0.01f, 0.00f, 0.01f},
    {240, 3, 0, -0.01f, 0.03f, 0.01f, 2.1f, -0.01f, 0.01f, -0.01f, 0.01f},
    {620, 1, 0, -0.02f, 0.04f, 0.01f, 1.1f, -0.01f, 0.01f, -0.01f, 0.01f},
    {900, 0, 0, 0.00f, 0.00f, 0.00f, 0.0f, 0.00f, 0.00f, 0.00f, 0.00f},
};

FaceClipKeyframe kShyRetractFrames[] = {
    {0, -5, 0, -0.06f, 0.10f, 0.05f, -3.4f, -0.06f, -0.02f, 0.03f, -0.01f},
    {180, -7, 1, -0.12f, 0.16f, 0.08f, -5.0f, -0.10f, -0.04f, 0.05f, -0.02f},
    {420, -3, 0, -0.07f, 0.10f, 0.05f, -2.6f, -0.05f, -0.02f, 0.03f, -0.01f},
    {700, 0, 0, 0.00f, 0.00f, 0.00f, 0.0f, 0.00f, 0.00f, 0.00f, 0.00f},
};

FaceClipKeyframe kHappyAckFrames[] = {
    {0, 0, 0, 0.05f, -0.06f, -0.02f, 0.8f, 0.03f, 0.03f, 0.02f, 0.01f},
    {120, 0, -1, 0.08f, -0.08f, -0.02f, 2.0f, 0.04f, 0.04f, 0.03f, 0.01f},
    {280, 0, 0, 0.03f, -0.04f, -0.01f, 1.0f, 0.02f, 0.02f, 0.01f, 0.00f},
    {520, 0, 0, 0.00f, 0.00f, 0.00f, 0.0f, 0.00f, 0.00f, 0.00f, 0.00f},
};

const FaceClipDefinition kDefinitions[] = {
    {FaceClipKind::WakeUp, "wake_up", 900, 1200, false, true, true, kWakeUpFrames, sizeof(kWakeUpFrames) / sizeof(kWakeUpFrames[0])},
    {FaceClipKind::GoToSleep, "go_to_sleep", 1100, 1400, true, true, true, kGoToSleepFrames, sizeof(kGoToSleepFrames) / sizeof(kGoToSleepFrames[0])},
    {FaceClipKind::AttentionRecovery, "attention_recovery", 620, 900, true, true, true, kAttentionRecoveryFrames, sizeof(kAttentionRecoveryFrames) / sizeof(kAttentionRecoveryFrames[0])},
    {FaceClipKind::ThinkingLoop, "thinking_loop", 1400, 950, true, false, true, kThinkingLoopFrames, sizeof(kThinkingLoopFrames) / sizeof(kThinkingLoopFrames[0])},
    {FaceClipKind::SoftListen, "soft_listen", 900, 850, true, false, true, kSoftListenFrames, sizeof(kSoftListenFrames) / sizeof(kSoftListenFrames[0])},
    {FaceClipKind::ShyRetract, "shy_retract", 700, 1000, true, false, true, kShyRetractFrames, sizeof(kShyRetractFrames) / sizeof(kShyRetractFrames[0])},
    {FaceClipKind::HappyAck, "happy_ack", 520, 600, true, false, true, kHappyAckFrames, sizeof(kHappyAckFrames) / sizeof(kHappyAckFrames[0])},
};

float lerp(float a, float b, float t) {
  return a + ((b - a) * t);
}

int lerpI(int a, int b, float t) {
  return static_cast<int>(static_cast<float>(a) + ((static_cast<float>(b) - static_cast<float>(a)) * t));
}

void copyFrame(const FaceClipKeyframe& frame, FaceClipSample* outSample) {
  outSample->offsetX = frame.offsetX;
  outSample->offsetY = frame.offsetY;
  outSample->opennessDelta = frame.opennessDelta;
  outSample->upperLidDelta = frame.upperLidDelta;
  outSample->lowerLidDelta = frame.lowerLidDelta;
  outSample->tiltDegDelta = frame.tiltDegDelta;
  outSample->squashYDelta = frame.squashYDelta;
  outSample->stretchXDelta = frame.stretchXDelta;
  outSample->roundnessDelta = frame.roundnessDelta;
  outSample->spacingScaleDelta = frame.spacingScaleDelta;
}

} // namespace

const FaceClipDefinition* faceClipDefinition(FaceClipKind kind) {
  for (size_t i = 0; i < (sizeof(kDefinitions) / sizeof(kDefinitions[0])); ++i) {
    if (kDefinitions[i].kind == kind) {
      return &kDefinitions[i];
    }
  }
  return nullptr;
}

const char* faceClipName(FaceClipKind kind) {
  const FaceClipDefinition* def = faceClipDefinition(kind);
  return def == nullptr ? "none" : def->name;
}

bool sampleFaceClip(const FaceClipDefinition& definition,
                    unsigned long elapsedMs,
                    FaceClipSample* outSample,
                    float* outPhaseNorm) {
  if (outSample == nullptr || outPhaseNorm == nullptr) {
    return false;
  }

  outSample->clear();
  *outPhaseNorm = 0.0f;

  if (definition.durationMs < 300 || definition.durationMs > 1500 ||
      definition.keyframes == nullptr || definition.keyframeCount < 2) {
    return false;
  }

  if (elapsedMs >= definition.durationMs) {
    return false;
  }

  const unsigned long clampedElapsed = elapsedMs > definition.durationMs ? definition.durationMs : elapsedMs;
  *outPhaseNorm = MathUtils::clamp(static_cast<float>(clampedElapsed) / static_cast<float>(definition.durationMs), 0.0f, 1.0f);

  if (clampedElapsed <= definition.keyframes[0].atMs) {
    copyFrame(definition.keyframes[0], outSample);
    return true;
  }

  const size_t last = definition.keyframeCount - 1;
  if (clampedElapsed >= definition.keyframes[last].atMs) {
    copyFrame(definition.keyframes[last], outSample);
    return true;
  }

  for (size_t i = 0; i < last; ++i) {
    const FaceClipKeyframe& a = definition.keyframes[i];
    const FaceClipKeyframe& b = definition.keyframes[i + 1];

    if (clampedElapsed < a.atMs || clampedElapsed > b.atMs) {
      continue;
    }

    const unsigned long span = b.atMs - a.atMs;
    const float t = span == 0 ? 0.0f : MathUtils::clamp(static_cast<float>(clampedElapsed - a.atMs) / static_cast<float>(span), 0.0f, 1.0f);

    outSample->offsetX = lerpI(a.offsetX, b.offsetX, t);
    outSample->offsetY = lerpI(a.offsetY, b.offsetY, t);
    outSample->opennessDelta = lerp(a.opennessDelta, b.opennessDelta, t);
    outSample->upperLidDelta = lerp(a.upperLidDelta, b.upperLidDelta, t);
    outSample->lowerLidDelta = lerp(a.lowerLidDelta, b.lowerLidDelta, t);
    outSample->tiltDegDelta = lerp(a.tiltDegDelta, b.tiltDegDelta, t);
    outSample->squashYDelta = lerp(a.squashYDelta, b.squashYDelta, t);
    outSample->stretchXDelta = lerp(a.stretchXDelta, b.stretchXDelta, t);
    outSample->roundnessDelta = lerp(a.roundnessDelta, b.roundnessDelta, t);
    outSample->spacingScaleDelta = lerp(a.spacingScaleDelta, b.spacingScaleDelta, t);
    return true;
  }

  return false;
}
