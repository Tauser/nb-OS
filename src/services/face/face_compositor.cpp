#include "face_compositor.h"

namespace {
unsigned long elapsedSince(unsigned long nowMs, unsigned long sinceMs) {
  return (nowMs >= sinceMs) ? (nowMs - sinceMs) : 0;
}
}

void FaceCompositor::init(unsigned long nowMs) {
  current_ = FaceRenderState{};
  current_.timestampMs = nowMs;
  initialized_ = true;
  lastBaseAcceptedMs_ = nowMs;
  lastClipAcceptedMs_ = nowMs;
  lastTransientAcceptedMs_ = nowMs;
}

void FaceCompositor::setPolicy(const FaceCompositorPolicy& policy) {
  policy_ = policy;
}

const FaceCompositorPolicy& FaceCompositor::policy() const {
  return policy_;
}

bool FaceCompositor::isClipActive(const FaceClipState& clip, unsigned long nowMs) {
  if (!clip.active || clip.kind == FaceClipKind::None) {
    return false;
  }

  if (clip.durationMs == 0) {
    return true;
  }

  return elapsedSince(nowMs, clip.startedAtMs) < clip.durationMs;
}

bool FaceCompositor::isTransientActive(const FaceTransientReactionState& transient, unsigned long nowMs) {
  if (!transient.active || transient.kind == FaceTransientReactionKind::None) {
    return false;
  }

  if (transient.durationMs == 0) {
    return true;
  }

  return elapsedSince(nowMs, transient.startedAtMs) < transient.durationMs;
}

void FaceCompositor::enforceDefaults(FaceRenderState& state, unsigned long nowMs) const {
  state.timestampMs = nowMs;

  if (state.base.owner == FaceLayerOwner::None) {
    state.base.owner = FaceLayerOwner::BaseExpressionController;
  }
  if (state.gaze.owner == FaceLayerOwner::None) {
    state.gaze.owner = FaceLayerOwner::GazeController;
  }
  if (state.blink.owner == FaceLayerOwner::None) {
    state.blink.owner = FaceLayerOwner::BlinkController;
  }
  if (state.idle.owner == FaceLayerOwner::None) {
    state.idle.owner = FaceLayerOwner::IdleController;
  }
  if (state.mood.owner == FaceLayerOwner::None) {
    state.mood.owner = FaceLayerOwner::MoodController;
  }
  if (state.transient.owner == FaceLayerOwner::None) {
    state.transient.owner = FaceLayerOwner::TransientReactionController;
  }
  if (state.clip.owner == FaceLayerOwner::None) {
    state.clip.owner = FaceLayerOwner::ClipPlayer;
  }

  state.base.locked = state.base.holdUntilMs > nowMs;
}

FaceRenderState FaceCompositor::resolve(const FaceRenderState& proposed, unsigned long nowMs) {
  if (!initialized_) {
    init(nowMs);
  }

  FaceRenderState resolved = proposed;

  const bool currentBaseLocked = current_.base.locked && current_.base.holdUntilMs > nowMs;
  const bool requestedBaseChanged = resolved.base.expression != current_.base.expression;
  const bool transitionCoolingDown =
      elapsedSince(nowMs, lastBaseAcceptedMs_) < policy_.transitionCooldownMs;

  if (requestedBaseChanged && (currentBaseLocked || transitionCoolingDown)) {
    resolved.base = current_.base;
  } else {
    if (requestedBaseChanged) {
      lastBaseAcceptedMs_ = nowMs;
    }

    const unsigned long minHoldUntil = nowMs + policy_.baseMinHoldMs;
    if (resolved.base.holdUntilMs < minHoldUntil) {
      resolved.base.holdUntilMs = minHoldUntil;
    }
    resolved.base.locked = resolved.base.holdUntilMs > nowMs;
  }

  const bool currentClipAlive = isClipActive(current_.clip, nowMs);
  const bool requestedClipAlive = isClipActive(proposed.clip, nowMs);

  if (requestedClipAlive) {
    const bool clipChanged = !currentClipAlive || (proposed.clip.kind != current_.clip.kind);
    const bool clipCoolingDown =
        clipChanged && (elapsedSince(nowMs, lastClipAcceptedMs_) < policy_.clipCooldownMs);

    if (clipCoolingDown) {
      resolved.clip = currentClipAlive ? current_.clip : FaceClipState{};
    } else {
      resolved.clip = proposed.clip;
      if (resolved.clip.startedAtMs == 0) {
        resolved.clip.startedAtMs = nowMs;
      }
      resolved.clip.active = true;
      if (clipChanged) {
        lastClipAcceptedMs_ = nowMs;
      }
    }
  } else {
    resolved.clip = currentClipAlive ? current_.clip : FaceClipState{};
  }

  if (!isClipActive(resolved.clip, nowMs)) {
    resolved.clip = FaceClipState{};
  }

  const bool currentTransientAlive = isTransientActive(current_.transient, nowMs);
  const bool requestedTransientAlive = isTransientActive(proposed.transient, nowMs);

  if (requestedTransientAlive) {
    const bool transientChanged =
        !currentTransientAlive || (proposed.transient.kind != current_.transient.kind);
    const bool transientCoolingDown =
        transientChanged && (elapsedSince(nowMs, lastTransientAcceptedMs_) < policy_.transientCooldownMs);
    const bool blockedByClip = isClipActive(resolved.clip, nowMs);

    if (transientCoolingDown || blockedByClip) {
      resolved.transient = currentTransientAlive ? current_.transient : FaceTransientReactionState{};
    } else {
      resolved.transient = proposed.transient;
      if (resolved.transient.startedAtMs == 0) {
        resolved.transient.startedAtMs = nowMs;
      }
      if (resolved.transient.durationMs < policy_.transientMinDurationMs) {
        resolved.transient.durationMs = policy_.transientMinDurationMs;
      }
      resolved.transient.active = true;
      if (transientChanged) {
        lastTransientAcceptedMs_ = nowMs;
      }
    }
  } else {
    resolved.transient = currentTransientAlive ? current_.transient : FaceTransientReactionState{};
  }

  if (!isTransientActive(resolved.transient, nowMs)) {
    resolved.transient = FaceTransientReactionState{};
  }

  if (isClipActive(resolved.clip, nowMs) && resolved.clip.ownsGaze) {
    resolved.gaze.enabled = false;
    resolved.gaze.mode = FaceGazeMode::None;
    resolved.gaze.amplitude = 0.0f;
    resolved.gaze.targetXNorm = 0.0f;
    resolved.gaze.targetYNorm = 0.0f;
    resolved.gaze.holdUntilMs = 0;
  }

  if (isClipActive(resolved.clip, nowMs) && !resolved.clip.allowsBlink) {
    resolved.blink.active = false;
    resolved.blink.mode = FaceBlinkMode::Suppressed;
    resolved.blink.durationMs = 0;
  }

  if (isClipActive(resolved.clip, nowMs)) {
    resolved.idle.enabled = false;
    resolved.idle.intensity = 0.0f;
  }

  enforceDefaults(resolved, nowMs);
  current_ = resolved;
  return resolved;
}
