#pragma once

#include <stdint.h>

#include "expression_types.h"

// Semantic ownership of each facial layer. This is intentionally explicit so
// future geometry/gaze work does not leak arbitration into the renderer.
enum class FaceLayerOwner : uint8_t {
  None = 0,
  BaseExpressionController,
  GazeController,
  BlinkController,
  IdleController,
  MoodController,
  TransientReactionController,
  ClipPlayer
};

// Base state defines the intentional expression selected by the system.
// It is the stable anchor that modulation layers build on top of.
struct FaceBaseExpressionState {
  ExpressionType expression = ExpressionType::Neutral;
  EyeAnimPriority priority = EyeAnimPriority::Idle;
  bool locked = false;
  unsigned long holdUntilMs = 0;
  FaceLayerOwner owner = FaceLayerOwner::BaseExpressionController;
};

enum class FaceGazeMode : uint8_t {
  None = 0,
  Hold,
  Saccade,
  MicroSaccade,
  Scan,
  Recover
};

// Gaze is directional intent only. It should not own base expression.
struct FaceGazeState {
  bool enabled = false;
  FaceGazeMode mode = FaceGazeMode::None;
  float targetXNorm = 0.0f;
  float targetYNorm = 0.0f;
  float amplitude = 0.0f;
  unsigned long holdUntilMs = 0;
  FaceLayerOwner owner = FaceLayerOwner::GazeController;
};

enum class FaceBlinkMode : uint8_t {
  Auto = 0,
  Forced,
  Suppressed,
  ClipDriven
};

// Blink is a local eyelid controller. It may temporarily override lid motion,
// but should not replace expression selection.
struct FaceBlinkState {
  FaceBlinkMode mode = FaceBlinkMode::Auto;
  bool active = false;
  bool allowDuringClip = true;
  unsigned long startedAtMs = 0;
  unsigned long durationMs = 0;
  FaceLayerOwner owner = FaceLayerOwner::BlinkController;
};

enum class FaceIdleMode : uint8_t {
  None = 0,
  Breath,
  Drift,
  AttentionPulse,
  SleepySettle
};

// Idle is long-running low-priority modulation. It must never replace the
// base expression, only bias it visually.
struct FaceIdleState {
  bool enabled = true;
  FaceIdleMode mode = FaceIdleMode::Breath;
  float intensity = 0.0f;
  FaceLayerOwner owner = FaceLayerOwner::IdleController;
};

enum class FaceMoodEnvelope : uint8_t {
  Neutral = 0,
  Warm,
  Calm,
  Alert,
  Sleepy,
  LowEnergy
};

// Mood modulation is a semantic envelope derived from emotion/mood. It nudges
// the face, but does not decide the whole expression by itself.
struct FaceMoodModulationState {
  FaceMoodEnvelope envelope = FaceMoodEnvelope::Neutral;
  float energy = 0.5f;
  float valence = 0.0f;
  float attention = 0.5f;
  float curiosity = 0.5f;
  FaceLayerOwner owner = FaceLayerOwner::MoodController;
};

enum class FaceTransientReactionKind : uint8_t {
  None = 0,
  Notice,
  Acknowledge,
  Startle,
  ShyRetreat,
  Recovery
};

// Transient reactions are short-lived event responses. They should be bounded
// by duration and anti-spam rules.
struct FaceTransientReactionState {
  FaceTransientReactionKind kind = FaceTransientReactionKind::None;
  bool active = false;
  unsigned long startedAtMs = 0;
  unsigned long durationMs = 0;
  FaceLayerOwner owner = FaceLayerOwner::TransientReactionController;
};

enum class FaceClipKind : uint8_t {
  None = 0,
  WakeUp,
  GoToSleep,
  AttentionRecovery,
  ThinkingLoop,
  SoftListen,
  ShyRetract,
  HappyAck
};

// Clips are temporal authored sequences. They may own subsets of the face for
// a time window, but must still declare whether blink/gaze remain active.
struct FaceClipState {
  FaceClipKind kind = FaceClipKind::None;
  bool active = false;
  float phaseNorm = 0.0f;
  unsigned long startedAtMs = 0;
  unsigned long durationMs = 0;
  bool allowsBlink = true;
  bool ownsGaze = false;
  FaceLayerOwner owner = FaceLayerOwner::ClipPlayer;
};

// This is the architectural gate before Face Premium++. It is not final
// geometry data; it is the semantic snapshot that future geometry/gaze/
// composition code will consume.
struct FaceRenderState {
  unsigned long timestampMs = 0;
  FaceBaseExpressionState base{};
  FaceGazeState gaze{};
  FaceBlinkState blink{};
  FaceIdleState idle{};
  FaceMoodModulationState mood{};
  FaceTransientReactionState transient{};
  FaceClipState clip{};

  void clearTransient() {
    transient = FaceTransientReactionState{};
  }

  bool hasActiveClip() const {
    return clip.active && clip.kind != FaceClipKind::None;
  }
};
