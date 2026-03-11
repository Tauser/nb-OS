#include "face_clip_player.h"

void FaceClipPlayer::init(unsigned long nowMs) {
  activeDef_ = nullptr;
  state_ = FaceClipState{};
  sample_.clear();
  cooldownUntilMs_ = nowMs;
}

bool FaceClipPlayer::play(FaceClipKind kind, unsigned long nowMs, bool allowPreempt) {
  if (kind == FaceClipKind::None) {
    return false;
  }

  const FaceClipDefinition* definition = faceClipDefinition(kind);
  if (definition == nullptr) {
    return false;
  }

  if (!state_.active && nowMs < cooldownUntilMs_) {
    return false;
  }

  if (state_.active) {
    if (!allowPreempt || activeDef_ == nullptr || !activeDef_->preemptible) {
      return false;
    }
  }

  startClip(*definition, nowMs);
  return true;
}

bool FaceClipPlayer::cancel(unsigned long nowMs) {
  if (!state_.active) {
    return false;
  }

  const unsigned long cooldownMs = activeDef_ == nullptr ? 120UL : (activeDef_->cooldownMs / 2UL);
  stopClip(nowMs + cooldownMs);
  return true;
}

void FaceClipPlayer::update(unsigned long nowMs) {
  if (!state_.active || activeDef_ == nullptr) {
    sample_.clear();
    state_ = FaceClipState{};
    return;
  }

  const unsigned long elapsed = nowMs - state_.startedAtMs;
  float phaseNorm = 0.0f;
  FaceClipSample sampled;
  if (!sampleFaceClip(*activeDef_, elapsed, &sampled, &phaseNorm)) {
    stopClip(nowMs + activeDef_->cooldownMs);
    return;
  }

  sample_ = sampled;
  state_.phaseNorm = phaseNorm;
}

bool FaceClipPlayer::isActive() const {
  return state_.active;
}

FaceClipKind FaceClipPlayer::activeKind() const {
  return state_.kind;
}

unsigned long FaceClipPlayer::cooldownUntilMs() const {
  return cooldownUntilMs_;
}

const FaceClipState& FaceClipPlayer::state() const {
  return state_;
}

const FaceClipSample& FaceClipPlayer::sample() const {
  return sample_;
}

void FaceClipPlayer::startClip(const FaceClipDefinition& definition, unsigned long nowMs) {
  activeDef_ = &definition;

  state_.kind = definition.kind;
  state_.active = true;
  state_.phaseNorm = 0.0f;
  state_.startedAtMs = nowMs;
  state_.durationMs = definition.durationMs;
  state_.allowsBlink = definition.allowsBlink;
  state_.ownsGaze = definition.ownsGaze;
  state_.owner = FaceLayerOwner::ClipPlayer;

  sample_.clear();
}

void FaceClipPlayer::stopClip(unsigned long cooldownUntilMs) {
  activeDef_ = nullptr;
  state_ = FaceClipState{};
  sample_.clear();
  cooldownUntilMs_ = cooldownUntilMs;
}
