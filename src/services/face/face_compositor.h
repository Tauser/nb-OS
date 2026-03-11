#pragma once

#include "../../models/face_render_state.h"

struct FaceCompositorPolicy {
  unsigned long baseMinHoldMs = 700;
  unsigned long transitionCooldownMs = 260;
  unsigned long clipCooldownMs = 1200;
  unsigned long transientCooldownMs = 650;
  unsigned long transientMinDurationMs = 180;
};

class FaceCompositor {
public:
  void init(unsigned long nowMs);
  void setPolicy(const FaceCompositorPolicy& policy);
  const FaceCompositorPolicy& policy() const;

  FaceRenderState resolve(const FaceRenderState& proposed, unsigned long nowMs);

private:
  FaceRenderState current_{};
  FaceCompositorPolicy policy_{};

  bool initialized_ = false;
  unsigned long lastBaseAcceptedMs_ = 0;
  unsigned long lastClipAcceptedMs_ = 0;
  unsigned long lastTransientAcceptedMs_ = 0;

  static bool isClipActive(const FaceClipState& clip, unsigned long nowMs);
  static bool isTransientActive(const FaceTransientReactionState& transient, unsigned long nowMs);
  void enforceDefaults(FaceRenderState& state, unsigned long nowMs) const;
};
