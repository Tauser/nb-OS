#pragma once

#include <stddef.h>
#include <stdint.h>

#include "face_render_state.h"

struct FaceClipKeyframe {
  uint16_t atMs = 0;

  int16_t offsetX = 0;
  int16_t offsetY = 0;

  float opennessDelta = 0.0f;
  float upperLidDelta = 0.0f;
  float lowerLidDelta = 0.0f;
  float tiltDegDelta = 0.0f;
  float squashYDelta = 0.0f;
  float stretchXDelta = 0.0f;
  float roundnessDelta = 0.0f;
  float spacingScaleDelta = 0.0f;

  FaceClipKeyframe() = default;
  FaceClipKeyframe(uint16_t at_ms,
                   int16_t offset_x,
                   int16_t offset_y,
                   float openness_delta,
                   float upper_lid_delta,
                   float lower_lid_delta,
                   float tilt_deg_delta,
                   float squash_y_delta,
                   float stretch_x_delta,
                   float roundness_delta,
                   float spacing_scale_delta)
      : atMs(at_ms),
        offsetX(offset_x),
        offsetY(offset_y),
        opennessDelta(openness_delta),
        upperLidDelta(upper_lid_delta),
        lowerLidDelta(lower_lid_delta),
        tiltDegDelta(tilt_deg_delta),
        squashYDelta(squash_y_delta),
        stretchXDelta(stretch_x_delta),
        roundnessDelta(roundness_delta),
        spacingScaleDelta(spacing_scale_delta) {}
};

struct FaceClipDefinition {
  FaceClipKind kind = FaceClipKind::None;
  const char* name = "none";
  uint16_t durationMs = 0;
  uint16_t cooldownMs = 0;
  bool allowsBlink = true;
  bool ownsGaze = false;
  bool preemptible = true;
  const FaceClipKeyframe* keyframes = nullptr;
  size_t keyframeCount = 0;

  FaceClipDefinition() = default;
  FaceClipDefinition(FaceClipKind clip_kind,
                     const char* clip_name,
                     uint16_t duration_ms,
                     uint16_t cooldown_ms,
                     bool allows_blink,
                     bool owns_gaze,
                     bool is_preemptible,
                     const FaceClipKeyframe* frames,
                     size_t frame_count)
      : kind(clip_kind),
        name(clip_name),
        durationMs(duration_ms),
        cooldownMs(cooldown_ms),
        allowsBlink(allows_blink),
        ownsGaze(owns_gaze),
        preemptible(is_preemptible),
        keyframes(frames),
        keyframeCount(frame_count) {}
};

struct FaceClipSample {
  int offsetX = 0;
  int offsetY = 0;

  float opennessDelta = 0.0f;
  float upperLidDelta = 0.0f;
  float lowerLidDelta = 0.0f;
  float tiltDegDelta = 0.0f;
  float squashYDelta = 0.0f;
  float stretchXDelta = 0.0f;
  float roundnessDelta = 0.0f;
  float spacingScaleDelta = 0.0f;

  void clear() {
    *this = FaceClipSample{};
  }
};

const FaceClipDefinition* faceClipDefinition(FaceClipKind kind);
const char* faceClipName(FaceClipKind kind);

// Samples a clip at elapsed time. Returns false when clip is no longer active.
bool sampleFaceClip(const FaceClipDefinition& definition,
                    unsigned long elapsedMs,
                    FaceClipSample* outSample,
                    float* outPhaseNorm);
