#pragma once

#include "../../models/face_clip.h"

class FaceClipPlayer {
public:
  void init(unsigned long nowMs);

  bool play(FaceClipKind kind, unsigned long nowMs, bool allowPreempt = true);
  bool cancel(unsigned long nowMs);
  void update(unsigned long nowMs);

  bool isActive() const;
  FaceClipKind activeKind() const;
  unsigned long cooldownUntilMs() const;

  const FaceClipState& state() const;
  const FaceClipSample& sample() const;

private:
  void startClip(const FaceClipDefinition& definition, unsigned long nowMs);
  void stopClip(unsigned long nowMs);

  const FaceClipDefinition* activeDef_ = nullptr;
  FaceClipState state_{};
  FaceClipSample sample_{};
  unsigned long cooldownUntilMs_ = 0;
};
