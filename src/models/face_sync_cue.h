#pragma once

#include <stdint.h>

// Internal multimodal hints emitted by motion sync. They are intentionally
// lightweight so LED/audio can react without tightly coupling to face internals.
enum class FaceSyncCue : uint8_t {
  None = 0,
  GazeLeft,
  GazeRight,
  GazeCenter,
  ClipWakeUp,
  ClipGoToSleep,
  ClipAttentionRecovery,
  ClipThinkingLoop,
  ClipSoftListen,
  ClipShyRetract,
  ClipHappyAck,
  ClipEnded
};

inline const char* faceSyncCueName(FaceSyncCue cue) {
  switch (cue) {
    case FaceSyncCue::GazeLeft: return "gaze_left";
    case FaceSyncCue::GazeRight: return "gaze_right";
    case FaceSyncCue::GazeCenter: return "gaze_center";
    case FaceSyncCue::ClipWakeUp: return "clip_wake_up";
    case FaceSyncCue::ClipGoToSleep: return "clip_go_to_sleep";
    case FaceSyncCue::ClipAttentionRecovery: return "clip_attention_recovery";
    case FaceSyncCue::ClipThinkingLoop: return "clip_thinking_loop";
    case FaceSyncCue::ClipSoftListen: return "clip_soft_listen";
    case FaceSyncCue::ClipShyRetract: return "clip_shy_retract";
    case FaceSyncCue::ClipHappyAck: return "clip_happy_ack";
    case FaceSyncCue::ClipEnded: return "clip_ended";
    case FaceSyncCue::None:
    default:
      return "none";
  }
}