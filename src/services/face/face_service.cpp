#include "face_service.h"
#include "face_geometry.h"

#include "../../config/hardware_config.h"
#include "../../utils/math_utils.h"
#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

namespace {
#ifndef NCOS_SIM_MODE
#define NCOS_SIM_MODE 0
#endif

constexpr unsigned long kPerfWindowMs = 5000;
constexpr unsigned long kEmotionOutputIntervalMs = HardwareConfig::EmotionOutput::FACE_APPLY_INTERVAL_MS;

float lerpF(float a, float b, float t) {
  return a + ((b - a) * t);
}

#if NCOS_SIM_MODE
const char* expressionName(ExpressionType e) {
  switch (e) {
    case ExpressionType::Neutral: return "neutral";
    case ExpressionType::Curiosity: return "curious";
    case ExpressionType::FaceRecognized: return "happy";
    case ExpressionType::BatteryAlert: return "alert";
    case ExpressionType::Angry: return "angry";
    case ExpressionType::Sad: return "sad";
    case ExpressionType::Surprised: return "surprised";
    case ExpressionType::Shy: return "shy";
    case ExpressionType::Listening: return "listening";
    case ExpressionType::Thinking: return "thinking";
    case ExpressionType::Blink: return "blink";
    default: return "unknown";
  }
}
#endif
bool parseTunerPresetName(const char* presetName, FacePresetId* outId) {
  return parseFacePresetId(presetName, outId);
}

const char* tunerPresetName(FacePresetId id) {
  return facePresetName(id);
}

const char* gazeModeName(FaceGazeMode mode) {
  switch (mode) {
    case FaceGazeMode::Hold: return "hold";
    case FaceGazeMode::Saccade: return "saccade";
    case FaceGazeMode::MicroSaccade: return "micro";
    case FaceGazeMode::Scan: return "scan";
    case FaceGazeMode::Recover: return "recover";
    case FaceGazeMode::None:
    default:
      return "none";
  }
}
}

FaceService::FaceService(IDisplayPort& displayPort, const IEmotionProvider& emotionProvider)
  : displayPort_(displayPort), emotionProvider_(emotionProvider), renderer_(displayPort) {
}

void FaceService::setMotionStateProvider(const IMotionStateProvider* motionStateProvider) {
  motionStateProvider_ = motionStateProvider;
}

void FaceService::init() {
  randomSeed(millis());

  leftEye_.centerX = HardwareConfig::Face::LEFT_EYE_X;
  leftEye_.centerY = HardwareConfig::Face::EYE_Y;
  leftEye_.eyeRadius = HardwareConfig::Face::EYE_RADIUS;

  rightEye_.centerX = HardwareConfig::Face::RIGHT_EYE_X;
  rightEye_.centerY = HardwareConfig::Face::EYE_Y;
  rightEye_.eyeRadius = HardwareConfig::Face::EYE_RADIUS;

  targetLeftEye_ = leftEye_;
  targetRightEye_ = rightEye_;

  displayPort_.init();

  const EmotionState& emo = emotionProvider_.getEmotionState();
  activePresetId_ = choosePreset(currentExpression_, emo);
  blendFromPresetId_ = activePresetId_;
  blendToPresetId_ = activePresetId_;
  tunerPresetId_ = activePresetId_;
  tunerPreset_ = getExpressionPreset(tunerPresetId_);
#if NCOS_SIM_MODE
  const char* presetValidationReason = nullptr;
  if (!validateFacePresetLibrary(&presetValidationReason)) {
    Serial.print("[WARN] [FACE_PRESET] validation_failed=");
    Serial.println(presetValidationReason == nullptr ? "unknown" : presetValidationReason);
  } else {
    Serial.println("[INFO] [FACE_PRESET] library_frozen=on");
  }
#endif

  const unsigned long now = millis();

  gazeController_.init(now);
  gazeController_.setContext(currentExpression_, emo.attention, emo.curiosity, true);
  clipPlayer_.init(now);
  clipPlayer_.play(FaceClipKind::WakeUp, now, true);
  clipPlayer_.update(now);


  FaceCompositorPolicy composePolicy;
  composePolicy.baseMinHoldMs = HardwareConfig::FaceComposition::BASE_MIN_HOLD_MS;
  composePolicy.transitionCooldownMs = HardwareConfig::FaceComposition::TRANSITION_COOLDOWN_MS;
  composePolicy.clipCooldownMs = HardwareConfig::FaceComposition::CLIP_COOLDOWN_MS;
  composePolicy.transientCooldownMs = HardwareConfig::FaceComposition::TRANSIENT_COOLDOWN_MS;
  composePolicy.transientMinDurationMs = HardwareConfig::FaceComposition::TRANSIENT_MIN_DURATION_MS;
  compositor_.setPolicy(composePolicy);
  compositor_.init(now);

  scheduleNextBlink(now);
  lastIdleMotionMs_ = now;
  lastSmoothMs_ = now;
  lastEmotionOutputMs_ = now;
  lastEmotionSwitchMs_ = now;
  lastExpressionChangeMs_ = now;
  lastPresetSwitchMs_ = now;
  perf_.windowStartMs = now;
  idleLastUpdateMs_ = now;
  nextIdleMicroAnimAtMs_ = now + static_cast<unsigned long>(random(2200, 5200));

  clipPlayer_.update(now);
  FaceLayerState layer = buildBaseExpressionLayer(now, emo);
  applyMoodModulationLayer(layer, emo);
  applyAttentionModulationLayer(layer, emo);
  applyNeckCoherenceLayer(layer, emo);

  const FaceLayerState baseLayer = layer;
  FaceLayerState layerBefore = layer;

  applyGazeControllerLayer(layer, now, emo);
  const FaceLayerDelta gazeDelta = captureDelta(layerBefore, layer);

  layerBefore = layer;
  applyIdleModulationLayer(layer, now, emo);
  const FaceLayerDelta idleDelta = captureDelta(layerBefore, layer);

  layerBefore = layer;
  applyTransientReactionLayer(layer, now, emo);
  const FaceLayerDelta transientDelta = captureDelta(layerBefore, layer);

  layerBefore = layer;
  applyClipLayer(layer);
  const FaceLayerDelta clipDelta = captureDelta(layerBefore, layer);

  layerBefore = layer;
  applyBlinkLayer(layer, now, emo);
  const FaceLayerDelta blinkDelta = captureDelta(layerBefore, layer);

  syncRenderState(now, emo, layer);
  renderState_ = compositor_.resolve(renderState_, now);

  const FaceLayerState resolvedLayer =
      resolveLayerWithCompositor(baseLayer, gazeDelta, idleDelta, transientDelta, clipDelta, blinkDelta);
  commitLayerState(resolvedLayer);

  smoothEyes(now);
  renderer_.render(leftEye_, rightEye_);
}

void FaceService::requestExpression(ExpressionType expression, EyeAnimPriority priority, unsigned long holdMs) {
  const unsigned long now = millis();

  if (tunerEnabled_ && static_cast<int>(priority) < static_cast<int>(EyeAnimPriority::Critical)) {
    return;
  }

  const bool switchingExpression = (expression != currentExpression_) && (expression != requestedExpression_);
  const bool alertNeutralPair =
      ((currentExpression_ == ExpressionType::BatteryAlert && expression == ExpressionType::Neutral) ||
       (currentExpression_ == ExpressionType::Neutral && expression == ExpressionType::BatteryAlert));

  // Prevent long-running alert/neutral ping-pong even if competing services use high priorities.
  if (alertNeutralPair && now - lastExpressionChangeMs_ < 3200UL) {
    return;
  }

  if (switchingExpression &&
      static_cast<int>(priority) <= static_cast<int>(currentPriority_) &&
      now - lastExpressionChangeMs_ < HardwareConfig::EmotionOutput::FACE_SWITCH_GUARD_MS) {
    return;
  }

  const unsigned long activeHold = (expressionHoldUntilMs_ > autoHoldUntilMs_) ? expressionHoldUntilMs_ : autoHoldUntilMs_;
  if (activeHold > now) {
    const int activePriority = static_cast<int>(currentPriority_);
    const int pendingPriority = static_cast<int>(requestedPriority_);
    const int incomingPriority = static_cast<int>(priority);
    const int strongestActive = activePriority > pendingPriority ? activePriority : pendingPriority;
    if (incomingPriority < strongestActive) {
      return;
    }
  }

  requestedExpression_ = expression;
  requestedPriority_ = priority;
  expressionHoldUntilMs_ = holdMs > 0 ? (now + holdMs) : 0;
}


void FaceService::requestGazeTarget(const FaceGazeTarget& target) {
  gazeController_.setTarget(target, millis());
}

void FaceService::clearGazeTarget() {
  gazeController_.clearTarget(millis());
}

bool FaceService::requestClip(FaceClipKind kind, bool allowPreempt) {
  return clipPlayer_.play(kind, millis(), allowPreempt);
}

bool FaceService::cancelClip() {
  return clipPlayer_.cancel(millis());
}

bool FaceService::tunerSetEnabled(bool enabled) {
  tunerEnabled_ = enabled;
  if (tunerEnabled_) {
    tunerPresetId_ = blendToPresetId_;
    tunerPreset_ = getExpressionPreset(tunerPresetId_);
    beginPresetBlend(blendToPresetId_, tunerPresetId_, millis());
  }
  return true;
}

bool FaceService::tunerSetPreset(const char* presetName) {
  FacePresetId presetId = FacePresetId::Neutral;
  if (!parseTunerPresetName(presetName, &presetId)) {
    return false;
  }

  tunerPresetId_ = presetId;
  tunerPreset_ = getExpressionPreset(tunerPresetId_);
  if (tunerEnabled_) {
    beginPresetBlend(blendToPresetId_, tunerPresetId_, millis());
  }
  return true;
}

bool FaceService::tunerSetParam(const char* key, float value) {
  if (!tunerEnabled_ || key == nullptr) {
    return false;
  }

  const PresetTuningGuardrails& limits = tunerPreset_.tuning;

  if (strcmp(key, "width") == 0) {
    tunerPreset_.widthScale = MathUtils::clamp(value, limits.widthMin, limits.widthMax);
    return true;
  }
  if (strcmp(key, "height") == 0) {
    tunerPreset_.heightScale = MathUtils::clamp(value, limits.heightMin, limits.heightMax);
    return true;
  }
  if (strcmp(key, "round") == 0 || strcmp(key, "roundness") == 0) {
    tunerPreset_.roundness = MathUtils::clamp(value, limits.roundnessMin, limits.roundnessMax);
    return true;
  }
  if (strcmp(key, "tilt") == 0) {
    tunerPreset_.tiltDeg = MathUtils::clamp(value, -30.0f, 30.0f);
    return true;
  }
  if (strcmp(key, "space") == 0 || strcmp(key, "spacing") == 0) {
    tunerPreset_.spacingScale = MathUtils::clamp(value, 0.84f, 1.20f);
    return true;
  }
  if (strcmp(key, "upper") == 0) {
    tunerPreset_.upperLid = MathUtils::clamp(value, limits.upperLidMin, limits.upperLidMax);
    return true;
  }
  if (strcmp(key, "lower") == 0) {
    tunerPreset_.lowerLid = MathUtils::clamp(value, limits.lowerLidMin, limits.lowerLidMax);
    return true;
  }
  if (strcmp(key, "open") == 0 || strcmp(key, "openness") == 0) {
    tunerPreset_.baseOpenness = MathUtils::clamp(value, limits.opennessMin, limits.opennessMax);
    return true;
  }

  if (strcmp(key, "blink_min") == 0) {
    tunerPreset_.blink.minIntervalMs = static_cast<unsigned long>(MathUtils::clamp(value, 400.0f, 16000.0f));
    return true;
  }
  if (strcmp(key, "blink_max") == 0) {
    tunerPreset_.blink.maxIntervalMs = static_cast<unsigned long>(MathUtils::clamp(value, 600.0f, 24000.0f));
    return true;
  }
  if (strcmp(key, "blink_dur") == 0) {
    tunerPreset_.blink.durationMs = static_cast<unsigned long>(MathUtils::clamp(value, 60.0f, 800.0f));
    return true;
  }

  if (strcmp(key, "tr_resp") == 0) {
    tunerPreset_.transition.responseHz = MathUtils::clamp(value, 2.0f, 40.0f);
    return true;
  }
  if (strcmp(key, "tr_amin") == 0) {
    tunerPreset_.transition.alphaMin = MathUtils::clamp(value, 0.01f, 0.5f);
    return true;
  }
  if (strcmp(key, "tr_amax") == 0) {
    tunerPreset_.transition.alphaMax = MathUtils::clamp(value, 0.05f, 0.95f);
    return true;
  }
  if (strcmp(key, "tr_blend") == 0) {
    tunerPreset_.transition.blendMs = static_cast<unsigned long>(MathUtils::clamp(value, 30.0f, 2600.0f));
    return true;
  }

  return false;
}

bool FaceService::tunerReset() {
  tunerPreset_ = getExpressionPreset(tunerPresetId_);
  return true;
}

bool FaceService::tunerGetStatus(char* out, size_t outSize) const {
  if (out == nullptr || outSize < 8) {
    return false;
  }

  const int written = snprintf(out,
                               outSize,
                               "enabled=%s preset=%s tier=%s frozen=%s gaze=%s clip=%s width=%.3f height=%.3f round=%.3f tilt=%.2f space=%.3f upper=%.3f lower=%.3f open=%.3f",
                               tunerEnabled_ ? "on" : "off",
                               tunerPresetName(tunerPresetId_),
                               facePresetTierName(getExpressionPreset(tunerPresetId_).readabilityTier),
                               facePresetLibraryFrozen() ? "yes" : "no",
                               gazeModeName(renderState_.gaze.mode),
                               faceClipName(renderState_.clip.kind),
                               tunerPreset_.widthScale,
                               tunerPreset_.heightScale,
                               tunerPreset_.roundness,
                               tunerPreset_.tiltDeg,
                               tunerPreset_.spacingScale,
                               tunerPreset_.upperLid,
                               tunerPreset_.lowerLid,
                               tunerPreset_.baseOpenness);
  return written > 0;
}
const FaceRenderState& FaceService::getFaceRenderState() const {
  return renderState_;
}

FaceMoodEnvelope FaceService::chooseMoodEnvelope(const EmotionState& emo) const {
  if (emo.energy < 0.25f) {
    return FaceMoodEnvelope::LowEnergy;
  }
  if (emo.energy < 0.38f) {
    return FaceMoodEnvelope::Sleepy;
  }
  if (emo.attention > 0.72f || emo.arousal > 0.72f) {
    return FaceMoodEnvelope::Alert;
  }
  if (emo.valence > 0.25f) {
    return FaceMoodEnvelope::Warm;
  }
  if (emo.arousal < 0.35f) {
    return FaceMoodEnvelope::Calm;
  }
  return FaceMoodEnvelope::Neutral;
}

FaceTransientReactionKind FaceService::chooseTransientKind() const {
  switch (idleMicroAnim_) {
    case IdleMicroAnim::CuriosityPulse:
      return FaceTransientReactionKind::Notice;
    case IdleMicroAnim::TinySquish:
    case IdleMicroAnim::SlightRelax:
      return FaceTransientReactionKind::Acknowledge;
    case IdleMicroAnim::SleepySettling:
      return FaceTransientReactionKind::Recovery;
    case IdleMicroAnim::WakeCorrection:
      return FaceTransientReactionKind::Recovery;
    case IdleMicroAnim::None:
    default:
      return FaceTransientReactionKind::None;
  }
}

FaceIdleMode FaceService::chooseIdleMode() const {
  switch (idleMicroAnim_) {
    case IdleMicroAnim::SleepySettling:
      return FaceIdleMode::SleepySettle;
    case IdleMicroAnim::CuriosityPulse:
      return FaceIdleMode::AttentionPulse;
    case IdleMicroAnim::TinySquish:
    case IdleMicroAnim::SlightRelax:
    case IdleMicroAnim::WakeCorrection:
      return FaceIdleMode::Drift;
    case IdleMicroAnim::None:
    default:
      return FaceIdleMode::Breath;
  }
}

void FaceService::syncRenderState(unsigned long nowMs, const EmotionState& emo, const FaceLayerState& layer) {
  renderState_.timestampMs = nowMs;

  renderState_.base.expression = currentExpression_;
  renderState_.base.priority = currentPriority_;
  renderState_.base.holdUntilMs = (expressionHoldUntilMs_ > autoHoldUntilMs_) ? expressionHoldUntilMs_ : autoHoldUntilMs_;
  renderState_.base.locked = renderState_.base.holdUntilMs > nowMs;
  renderState_.base.owner = FaceLayerOwner::BaseExpressionController;

  const FaceGazeController::Output& gazeOut = gazeController_.output();
  renderState_.gaze.enabled = gazeOut.enabled;
  renderState_.gaze.mode = gazeOut.mode;
  renderState_.gaze.targetXNorm = gazeOut.targetXNorm;
  renderState_.gaze.targetYNorm = gazeOut.targetYNorm;
  renderState_.gaze.amplitude = gazeOut.amplitude;
  renderState_.gaze.holdUntilMs = gazeOut.fixationUntilMs;
  renderState_.gaze.owner = FaceLayerOwner::GazeController;

  renderState_.blink.active = blinking_;
  renderState_.blink.startedAtMs = blinkStartMs_;
  renderState_.blink.durationMs = blinkDurationMs_;
  renderState_.blink.allowDuringClip = true;
  renderState_.blink.mode = FaceBlinkMode::Auto;
  renderState_.blink.owner = FaceLayerOwner::BlinkController;

  renderState_.idle.enabled = true;
  renderState_.idle.mode = chooseIdleMode();
  renderState_.idle.intensity = MathUtils::clamp(fabsf(idleAttentionBias_) * 0.08f + fabsf(layer.stretchX - 1.0f) + fabsf(layer.squashY - 1.0f), 0.0f, 1.0f);
  renderState_.idle.owner = FaceLayerOwner::IdleController;

  renderState_.mood.envelope = chooseMoodEnvelope(emo);
  renderState_.mood.energy = emo.energy;
  renderState_.mood.valence = emo.valence;
  renderState_.mood.attention = emo.attention;
  renderState_.mood.curiosity = emo.curiosity;
  renderState_.mood.owner = FaceLayerOwner::MoodController;

  const FaceTransientReactionKind transientKind = chooseTransientKind();
  renderState_.transient.kind = transientKind;
  renderState_.transient.active = (transientKind != FaceTransientReactionKind::None);
  renderState_.transient.startedAtMs = idleMicroStartMs_;
  renderState_.transient.durationMs = idleMicroDurationMs_;
  renderState_.transient.owner = FaceLayerOwner::TransientReactionController;

  renderState_.clip = clipPlayer_.state();
  if (renderState_.clip.owner == FaceLayerOwner::None) {
    renderState_.clip.owner = FaceLayerOwner::ClipPlayer;
  }
}
void FaceService::update(unsigned long nowMs) {
  const EmotionState& emo = emotionProvider_.getEmotionState();
  clipPlayer_.update(nowMs);

  if (tunerEnabled_) {
    FaceLayerState layer = buildBaseExpressionLayer(nowMs, emo);
    syncRenderState(nowMs, emo, layer);
    renderState_ = compositor_.resolve(renderState_, nowMs);
    commitLayerState(layer);

    smoothEyes(nowMs);

    const unsigned long t0 = micros();
    const bool rendered = renderer_.render(leftEye_, rightEye_);
    const unsigned long renderUs = micros() - t0;
    recordPerf(nowMs, rendered, renderUs);
    return;
  }

  applyEmotionOutput(nowMs);
  stepStateMachine(nowMs);

  FaceLayerState layer = buildBaseExpressionLayer(nowMs, emo);
  applyMoodModulationLayer(layer, emo);
  applyAttentionModulationLayer(layer, emo);
  applyNeckCoherenceLayer(layer, emo);

  const FaceLayerState baseLayer = layer;
  FaceLayerState layerBefore = layer;

  applyGazeControllerLayer(layer, nowMs, emo);
  const FaceLayerDelta gazeDelta = captureDelta(layerBefore, layer);

  layerBefore = layer;
  applyIdleModulationLayer(layer, nowMs, emo);
  const FaceLayerDelta idleDelta = captureDelta(layerBefore, layer);

  layerBefore = layer;
  applyTransientReactionLayer(layer, nowMs, emo);
  const FaceLayerDelta transientDelta = captureDelta(layerBefore, layer);

  layerBefore = layer;
  applyClipLayer(layer);
  const FaceLayerDelta clipDelta = captureDelta(layerBefore, layer);

  layerBefore = layer;
  applyBlinkLayer(layer, nowMs, emo);
  const FaceLayerDelta blinkDelta = captureDelta(layerBefore, layer);

  syncRenderState(nowMs, emo, layer);
  renderState_ = compositor_.resolve(renderState_, nowMs);

  const FaceLayerState resolvedLayer =
      resolveLayerWithCompositor(baseLayer, gazeDelta, idleDelta, transientDelta, clipDelta, blinkDelta);
  commitLayerState(resolvedLayer);

  smoothEyes(nowMs);

  const unsigned long t0 = micros();
  const bool rendered = renderer_.render(leftEye_, rightEye_);
  const unsigned long renderUs = micros() - t0;
  recordPerf(nowMs, rendered, renderUs);
}

void FaceService::applyEmotionOutput(unsigned long nowMs) {
  if (nowMs - lastEmotionOutputMs_ < kEmotionOutputIntervalMs) {
    return;
  }
  lastEmotionOutputMs_ = nowMs;

  if (!HardwareConfig::EmotionOutput::FACE_ENABLE_EXPRESSION_FROM_EMOTION) {
    return;
  }

  const EmotionState& emo = emotionProvider_.getEmotionState();

  const float energyEnterLow = HardwareConfig::EmotionOutput::FACE_ENERGY_LOW;
  const float energyExitLow = energyEnterLow + 0.08f;
  const bool energyLow = (emo.energy < energyEnterLow) ||
                         (lastEmotionTarget_ == ExpressionType::BatteryAlert && emo.energy < energyExitLow);

  const float curiousEnter = HardwareConfig::EmotionOutput::FACE_CURIOSITY_HIGH;
  const float curiousExit = curiousEnter - 0.10f;
  const bool curiousHigh = (emo.curiosity > curiousEnter) ||
                           (lastEmotionTarget_ == ExpressionType::Curiosity && emo.curiosity > curiousExit);

  ExpressionType target = ExpressionType::Neutral;
  if (energyLow) {
    target = ExpressionType::BatteryAlert;
  } else if (emo.valence < HardwareConfig::EmotionOutput::FACE_VALENCE_NEG) {
    target = ExpressionType::Sad;
  } else if (curiousHigh) {
    target = ExpressionType::Curiosity;
  } else if (emo.valence > HardwareConfig::EmotionOutput::FACE_VALENCE_POS) {
    target = ExpressionType::FaceRecognized;
  }

  if (target == ExpressionType::Neutral) {
    return;
  }

  if (target != lastEmotionTarget_) {
    if (nowMs - lastEmotionSwitchMs_ < 2600UL) {
      return;
    }
    lastEmotionTarget_ = target;
    lastEmotionSwitchMs_ = nowMs;
  }

  const unsigned long activeHold = (expressionHoldUntilMs_ > autoHoldUntilMs_) ? expressionHoldUntilMs_ : autoHoldUntilMs_;
  if (target == currentExpression_ && nowMs < activeHold) {
    return;
  }

  requestExpression(target, EyeAnimPriority::Emotion, HardwareConfig::EmotionOutput::FACE_EXPRESSION_HOLD_MS);
}

void FaceService::stepStateMachine(unsigned long nowMs) {
  const unsigned long holdUntil = (expressionHoldUntilMs_ > autoHoldUntilMs_) ? expressionHoldUntilMs_ : autoHoldUntilMs_;
  if (holdUntil > 0 && nowMs > holdUntil) {
    requestedExpression_ = currentExpression_;
    requestedPriority_ = EyeAnimPriority::Idle;
    currentPriority_ = EyeAnimPriority::Idle;
    expressionHoldUntilMs_ = 0;
    autoHoldUntilMs_ = 0;
  }

  if (!blinking_ && static_cast<int>(requestedPriority_) >= static_cast<int>(currentPriority_)) {
    const ExpressionType prevExpression = currentExpression_;
    currentExpression_ = requestedExpression_;
    currentPriority_ = requestedPriority_;

    if (currentExpression_ != prevExpression) {
      lastExpressionChangeMs_ = nowMs;
      const FacePresetId nextPreset = choosePreset(currentExpression_, emotionProvider_.getEmotionState());
      beginPresetBlend(blendToPresetId_, nextPreset, nowMs);
      const ExpressionPreset& nextPresetData = getExpressionPreset(nextPreset);
      autoHoldUntilMs_ = nowMs + nextPresetData.transition.holdMs;
    }
      queueClipForExpressionTransition(prevExpression, currentExpression_, nowMs);

#if NCOS_SIM_MODE
    if (currentExpression_ != prevExpression) {
      Serial.print("[FACE_STATE] expr=");
      Serial.println(expressionName(currentExpression_));
    }
#endif
  }
}

void FaceService::queueClipForExpressionTransition(ExpressionType previous, ExpressionType current, unsigned long nowMs) {
  if (current == previous) {
    return;
  }

  FaceClipKind clipKind = FaceClipKind::None;
  bool allowPreempt = true;

  switch (current) {
    case ExpressionType::Surprised:
      clipKind = FaceClipKind::AttentionRecovery;
      break;
    case ExpressionType::Thinking:
      clipKind = FaceClipKind::ThinkingLoop;
      allowPreempt = false;
      break;
    case ExpressionType::Listening:
      clipKind = FaceClipKind::SoftListen;
      allowPreempt = false;
      break;
    case ExpressionType::Shy:
      clipKind = FaceClipKind::ShyRetract;
      break;
    case ExpressionType::FaceRecognized:
      clipKind = FaceClipKind::HappyAck;
      break;
    case ExpressionType::BatteryAlert:
      clipKind = FaceClipKind::GoToSleep;
      break;
    case ExpressionType::Curiosity:
      clipKind = (previous == ExpressionType::BatteryAlert) ? FaceClipKind::WakeUp : FaceClipKind::AttentionRecovery;
      break;
    default:
      break;
  }

  if (clipKind != FaceClipKind::None) {
    clipPlayer_.play(clipKind, nowMs, allowPreempt);
  }
}

void FaceService::beginPresetBlend(FacePresetId from, FacePresetId to, unsigned long nowMs) {
  blendFromPresetId_ = from;
  blendToPresetId_ = to;

  if (from == to) {
    presetBlendActive_ = false;
    presetBlendStartMs_ = nowMs;
    presetBlendDurationMs_ = 0;
    activePresetId_ = to;
    return;
  }

  const ExpressionPreset& fromPreset = getExpressionPreset(from);
  const ExpressionPreset& toPreset = getExpressionPreset(to);

  unsigned long duration = toPreset.transition.blendMs;
  if (fromPreset.transition.blendMs > duration) {
    duration = fromPreset.transition.blendMs;
  }
  if (duration < 80UL) {
    duration = 80UL;
  }

  presetBlendActive_ = true;
  presetBlendStartMs_ = nowMs;
  presetBlendDurationMs_ = duration;
}


float FaceService::computePresetBlend(unsigned long nowMs) const {
  if (!presetBlendActive_ || presetBlendDurationMs_ == 0) {
    return 1.0f;
  }

  const unsigned long elapsed = nowMs - presetBlendStartMs_;
  float t = static_cast<float>(elapsed) / static_cast<float>(presetBlendDurationMs_);
  t = MathUtils::clamp(t, 0.0f, 1.0f);

  const ExpressionPreset& toPreset = getExpressionPreset(blendToPresetId_);
  return easedBlend(t, toPreset.transition);
}

float FaceService::easedBlend(float t, const TransitionProfile& transition) const {
  const float clamped = MathUtils::clamp(t, 0.0f, 1.0f);
  const float smooth = easeInOutCubic(clamped);
  const float curve = MathUtils::clamp(transition.blendCurveExp, 0.70f, 2.20f);
  const float shaped = powf(smooth, curve);

  // Small soft landing near the end avoids "snap" when entering hold/settle.
  const float tail = (clamped > 0.86f) ? ((clamped - 0.86f) / 0.14f) : 0.0f;
  const float tailEase = (tail > 0.0f) ? (1.0f - ((1.0f - tail) * (1.0f - tail))) : 0.0f;
  return MathUtils::clamp((shaped * (1.0f - (0.08f * tailEase))) + (clamped * 0.08f * tailEase), 0.0f, 1.0f);
}

FaceService::FaceLayerState FaceService::buildBaseExpressionLayer(unsigned long nowMs, const EmotionState& emo) {
  FacePresetId desiredPresetId = tunerEnabled_ ? tunerPresetId_ : choosePreset(currentExpression_, emo);

  if (desiredPresetId != blendToPresetId_) {
    const PresetUsageProfile& desiredUsage = facePresetUsageProfile(desiredPresetId);
    const unsigned long dwellMs =
        desiredUsage.role == FacePresetUsageRole::Situational ? 180UL : 520UL;

    if (nowMs - lastPresetSwitchMs_ >= dwellMs) {
      beginPresetBlend(blendToPresetId_, desiredPresetId, nowMs);
      lastPresetSwitchMs_ = nowMs;
    }
  }

  const ExpressionPreset& fromPreset = (tunerEnabled_ && blendFromPresetId_ == tunerPresetId_)
                                          ? tunerPreset_
                                          : getExpressionPreset(blendFromPresetId_);
  const ExpressionPreset& toPreset = (tunerEnabled_ && blendToPresetId_ == tunerPresetId_)
                                        ? tunerPreset_
                                        : getExpressionPreset(blendToPresetId_);
  const float blend = computePresetBlend(nowMs);

  if (presetBlendActive_ && (nowMs - presetBlendStartMs_ >= presetBlendDurationMs_)) {
    presetBlendActive_ = false;
    activePresetId_ = blendToPresetId_;
    presetSettleUntilMs_ = nowMs + toPreset.transition.settleMs;
  } else {
    activePresetId_ = blendToPresetId_;
  }

  FaceLayerState layer;
  layer.spacingScale = lerpF(fromPreset.spacingScale, toPreset.spacingScale, blend);
  layer.stretchX = lerpF(fromPreset.widthScale, toPreset.widthScale, blend);
  layer.squashY = lerpF(fromPreset.heightScale, toPreset.heightScale, blend);
  layer.tiltDeg = lerpF(fromPreset.tiltDeg, toPreset.tiltDeg, blend);
  layer.roundness = lerpF(fromPreset.roundness, toPreset.roundness, blend);
  layer.upperLid = lerpF(fromPreset.upperLid, toPreset.upperLid, blend);
  layer.lowerLid = lerpF(fromPreset.lowerLid, toPreset.lowerLid, blend);
  layer.openness = lerpF(fromPreset.baseOpenness, toPreset.baseOpenness, blend);
  const int centerX = (HardwareConfig::Face::LEFT_EYE_X + HardwareConfig::Face::RIGHT_EYE_X) / 2;
  const int halfSpacing = static_cast<int>((HardwareConfig::Face::RIGHT_EYE_X - HardwareConfig::Face::LEFT_EYE_X) * 0.5f * layer.spacingScale);
  layer.leftX = centerX - halfSpacing;
  layer.rightX = centerX + halfSpacing;
  layer.y = HardwareConfig::Face::EYE_Y;

  return layer;
}

void FaceService::applyMoodModulationLayer(FaceLayerState& layer, const EmotionState& emo) {
  const float moodPos = MathUtils::clamp((emo.valence + 1.0f) * 0.5f, 0.0f, 1.0f);
  const float gain = facePresetLayerBlend(blendToPresetId_).moodGain;

  layer.openness += ((emo.curiosity - 0.5f) * 0.10f) * gain;
  layer.openness -= ((1.0f - emo.energy) * 0.22f) * gain;
  layer.openness += ((moodPos - 0.5f) * 0.06f) * gain;

  layer.stretchX += ((emo.curiosity - 0.5f) * 0.15f) * gain;
  layer.stretchX += ((moodPos - 0.5f) * 0.10f) * gain;

  layer.squashY += ((emo.attention - 0.5f) * 0.08f) * gain;
  layer.squashY -= ((1.0f - emo.energy) * 0.10f) * gain;

  layer.tiltDeg += (((emo.arousal - 0.5f) * 4.2f) + (emo.valence * 1.8f)) * gain;
  layer.roundness += ((moodPos - 0.5f) * 0.06f) * gain;
}

void FaceService::applyAttentionModulationLayer(FaceLayerState& layer, const EmotionState& emo) {
  const float gain = facePresetLayerBlend(blendToPresetId_).attentionGain;

  layer.upperLid += ((1.0f - emo.energy) * 0.08f) * gain;
  layer.lowerLid += ((1.0f - emo.energy) * 0.03f) * gain;
  layer.upperLid -= (emo.attention * 0.05f) * gain;

  layer.spacingScale += ((emo.attention - 0.5f) * 0.04f) * gain;
  layer.stretchX -= ((emo.attention - 0.5f) * 0.03f) * gain;
}

void FaceService::applyNeckCoherenceLayer(FaceLayerState& layer, const EmotionState& emo) {
  float neckYawDeg = 0.0f;
  float neckTiltDeg = 0.0f;
  MotionPoseId neckPoseId = MotionPoseId::Center;

  if (motionStateProvider_ != nullptr) {
    neckYawDeg = motionStateProvider_->getCurrentYawDeg();
    neckTiltDeg = motionStateProvider_->getCurrentTiltDeg();
    neckPoseId = motionStateProvider_->getCurrentPoseId();
  }

  layer.neckPoseId = neckPoseId;
  layer.yawNorm = MathUtils::clamp(neckYawDeg / HardwareConfig::Motion::YAW_MAX_DEG, -1.0f, 1.0f);
  layer.tiltNorm = MathUtils::clamp(neckTiltDeg / HardwareConfig::Motion::TILT_MAX_DEG, -1.0f, 1.0f);

  layer.spacingScale += fabsf(layer.yawNorm) * 0.025f;
  layer.stretchX -= fabsf(layer.yawNorm) * 0.040f;
  layer.tiltDeg += layer.yawNorm * 3.6f;

  layer.openness -= MathUtils::clamp(layer.tiltNorm, 0.0f, 1.0f) * 0.10f;
  layer.upperLid += MathUtils::clamp(layer.tiltNorm, 0.0f, 1.0f) * 0.06f;

  switch (neckPoseId) {
    case MotionPoseId::CuriousLeft:
    case MotionPoseId::CuriousRight:
      layer.stretchX += 0.050f;
      layer.openness += 0.060f;
      layer.roundness -= 0.040f;
      layer.tiltDeg += (neckPoseId == MotionPoseId::CuriousLeft) ? -2.2f : 2.2f;
      break;

    case MotionPoseId::SoftListen:
      layer.upperLid += 0.050f;
      layer.lowerLid += 0.020f;
      layer.stretchX -= 0.025f;
      layer.openness += 0.030f;
      break;

    case MotionPoseId::YawLeft:
    case MotionPoseId::YawRight:
      if (emo.arousal > 0.70f) {
        layer.openness += 0.090f;
        layer.stretchX += 0.060f;
        layer.upperLid -= 0.050f;
      }
      break;

    case MotionPoseId::TiltLeft:
    case MotionPoseId::TiltRight:
      if (emo.energy < 0.42f) {
        layer.openness -= 0.070f;
        layer.upperLid += 0.070f;
        layer.lowerLid += 0.020f;
      }
      break;

    case MotionPoseId::Center:
    case MotionPoseId::IdleSway:
    default:
      break;
  }

  const int centerX = (HardwareConfig::Face::LEFT_EYE_X + HardwareConfig::Face::RIGHT_EYE_X) / 2;
  const int halfSpacing = static_cast<int>((HardwareConfig::Face::RIGHT_EYE_X - HardwareConfig::Face::LEFT_EYE_X) * 0.5f * layer.spacingScale);
  const int gazeOffsetX = static_cast<int>(layer.yawNorm * (3.5f + (emo.attention * 1.5f)));
  const int gazeOffsetY = static_cast<int>(MathUtils::clamp(layer.tiltNorm, -1.0f, 1.0f) * 2.0f);

  layer.leftX = centerX - halfSpacing + gazeOffsetX;
  layer.rightX = centerX + halfSpacing + gazeOffsetX;
  layer.y = HardwareConfig::Face::EYE_Y + gazeOffsetY;
}

void FaceService::applyGazeControllerLayer(FaceLayerState& layer, unsigned long nowMs, const EmotionState& emo) {
  const bool idleLike = (currentExpression_ == ExpressionType::Neutral) ||
                        (currentExpression_ == ExpressionType::Curiosity) ||
                        (currentExpression_ == ExpressionType::BatteryAlert) ||
                        (currentExpression_ == ExpressionType::Sad);

  gazeController_.setContext(currentExpression_, emo.attention, emo.curiosity, idleLike);
  gazeController_.update(nowMs);

  const FaceGazeController::Output& gazeOut = gazeController_.output();
  if (!gazeOut.enabled) {
    return;
  }

  const float gain = facePresetLayerBlend(blendToPresetId_).gazeGain;

  const int offsetX = static_cast<int>(gazeOut.currentXNorm * (5.0f + (emo.attention * 2.0f)) * gain);
  const int offsetY = static_cast<int>(gazeOut.currentYNorm * (3.0f + (emo.attention * 1.2f)) * gain);

  layer.leftX += offsetX;
  layer.rightX += offsetX;
  layer.y += offsetY;

  layer.yawNorm = MathUtils::clamp(layer.yawNorm + ((gazeOut.currentXNorm * 0.65f) * gain), -1.0f, 1.0f);
  layer.tiltNorm = MathUtils::clamp(layer.tiltNorm + ((gazeOut.currentYNorm * 0.60f) * gain), -1.0f, 1.0f);

  layer.tiltDeg += (gazeOut.currentXNorm * 1.6f) * gain;
  layer.spacingScale += (fabsf(gazeOut.currentXNorm) * 0.015f) * gain;
}
void FaceService::applyIdleModulationLayer(FaceLayerState& layer, unsigned long nowMs, const EmotionState& emo) {
  const unsigned long dtMs = (idleLastUpdateMs_ == 0) ? 0 : (nowMs - idleLastUpdateMs_);
  idleLastUpdateMs_ = nowMs;

  if (nowMs - lastIdleMotionMs_ < HardwareConfig::Face::LOOK_INTERVAL_MS) {
    return;
  }

  lastIdleMotionMs_ = nowMs;

  const float dt = MathUtils::clamp(static_cast<float>(dtMs) / 1000.0f, 0.0f, 0.050f);
  const float energy = MathUtils::clamp(emo.energy, 0.0f, 1.0f);
  const float arousal = MathUtils::clamp(emo.arousal, 0.0f, 1.0f);
  const float attention = MathUtils::clamp(emo.attention, 0.0f, 1.0f);
  const float gain = facePresetLayerBlend(blendToPresetId_).idleGain;

  idleBreathPhase_ += dt * (0.85f + (0.55f * energy));
  idleDriftPhase_ += dt * (0.42f + (0.30f * emo.curiosity));
  idleAttentionPhase_ += dt * (0.65f + (0.80f * attention));

  const float breath = sinf(idleBreathPhase_);
  const float drift = sinf(idleDriftPhase_);
  const float attentionPulse = sinf(idleAttentionPhase_);

  const float activity = MathUtils::clamp((arousal * HardwareConfig::EmotionOutput::FACE_ACTIVITY_AROUSAL_WEIGHT) +
                                              (energy * HardwareConfig::EmotionOutput::FACE_ACTIVITY_ENERGY_WEIGHT),
                                          0.0f,
                                          1.0f);

  layer.openness += ((activity - 0.5f) * HardwareConfig::EmotionOutput::FACE_ACTIVITY_GAIN) * gain;
  layer.openness -= ((1.0f - energy) * HardwareConfig::EmotionOutput::FACE_LOW_ENERGY_GAIN) * gain;
  layer.openness += (breath * (0.008f + ((1.0f - energy) * 0.010f))) * gain;

  const float breathHeight = (breath * (0.008f + ((1.0f - energy) * 0.010f))) * gain;
  const float driftWidth = (drift * (0.010f + (attention * 0.004f))) * gain;
  const float tiltDrift = (drift * (0.55f + (emo.curiosity * 0.35f))) * gain;

  layer.squashY += breathHeight;
  layer.stretchX += driftWidth;
  layer.tiltDeg += tiltDrift;

  idleAttentionBias_ = lerpF(idleAttentionBias_, (attentionPulse * 0.6f) * gain, 0.18f);
  layer.leftX += static_cast<int>(idleAttentionBias_);
  layer.rightX += static_cast<int>(idleAttentionBias_);
}

void FaceService::startIdleMicroAnim(unsigned long nowMs, const EmotionState& emo) {
  idleMicroStartMs_ = nowMs;

  const bool sleepy = emo.energy < 0.32f;
  const bool curious = emo.curiosity > 0.66f;
  const bool recoveringAttention = (nowMs - lastExpressionChangeMs_) > 18000UL && emo.attention > 0.52f;

  if (recoveringAttention) {
    idleMicroAnim_ = IdleMicroAnim::WakeCorrection;
    idleMicroDurationMs_ = static_cast<unsigned long>(random(700, 1100));
  } else if (sleepy) {
    idleMicroAnim_ = IdleMicroAnim::SleepySettling;
    idleMicroDurationMs_ = static_cast<unsigned long>(random(1300, 1900));
  } else if (curious) {
    idleMicroAnim_ = IdleMicroAnim::CuriosityPulse;
    idleMicroDurationMs_ = static_cast<unsigned long>(random(850, 1250));
  } else if (random(0, 100) < 50) {
    idleMicroAnim_ = IdleMicroAnim::TinySquish;
    idleMicroDurationMs_ = static_cast<unsigned long>(random(520, 900));
  } else {
    idleMicroAnim_ = IdleMicroAnim::SlightRelax;
    idleMicroDurationMs_ = static_cast<unsigned long>(random(800, 1300));
  }

  nextIdleMicroAnimAtMs_ = nowMs + static_cast<unsigned long>(random(2300, 5600));
}

float FaceService::idleMicroEnvelope(unsigned long nowMs) const {
  if (idleMicroAnim_ == IdleMicroAnim::None || idleMicroDurationMs_ == 0) {
    return 0.0f;
  }

  const unsigned long elapsed = nowMs - idleMicroStartMs_;
  if (elapsed >= idleMicroDurationMs_) {
    return 0.0f;
  }

  const float t = static_cast<float>(elapsed) / static_cast<float>(idleMicroDurationMs_);
  return sinf(3.14159f * t);
}

void FaceService::applyTransientReactionLayer(FaceLayerState& layer, unsigned long nowMs, const EmotionState& emo) {
  if (nowMs >= nextIdleMicroAnimAtMs_ && idleMicroAnim_ == IdleMicroAnim::None) {
    startIdleMicroAnim(nowMs, emo);
  }

  const float transientGain = facePresetLayerBlend(blendToPresetId_).transientGain;
  const float microEnv = idleMicroEnvelope(nowMs) * transientGain;
  if (microEnv <= 0.0001f) {
    idleMicroAnim_ = IdleMicroAnim::None;
    return;
  }

  switch (idleMicroAnim_) {
    case IdleMicroAnim::TinySquish:
      layer.squashY -= 0.055f * microEnv;
      layer.stretchX += 0.030f * microEnv;
      break;

    case IdleMicroAnim::SlightRelax:
      layer.upperLid += 0.060f * microEnv;
      layer.lowerLid += 0.020f * microEnv;
      break;

    case IdleMicroAnim::SleepySettling:
      layer.upperLid += 0.120f * microEnv;
      layer.squashY -= 0.070f * microEnv;
      break;

    case IdleMicroAnim::CuriosityPulse:
      layer.stretchX += 0.045f * microEnv;
      layer.roundness -= 0.045f * microEnv;
      break;

    case IdleMicroAnim::WakeCorrection:
      layer.tiltDeg += 2.0f * microEnv;
      layer.upperLid -= 0.050f * microEnv;
      break;

    case IdleMicroAnim::None:
    default:
      break;
  }
}

void FaceService::applyClipLayer(FaceLayerState& layer) {
  if (!clipPlayer_.isActive()) {
    return;
  }

  const float gain = facePresetLayerBlend(blendToPresetId_).clipGain;
  const FaceClipSample& clip = clipPlayer_.sample();
  layer.leftX += static_cast<int>(clip.offsetX * gain);
  layer.rightX += static_cast<int>(clip.offsetX * gain);
  layer.y += static_cast<int>(clip.offsetY * gain);
  layer.openness += clip.opennessDelta * gain;
  layer.upperLid += clip.upperLidDelta * gain;
  layer.lowerLid += clip.lowerLidDelta * gain;
  layer.tiltDeg += clip.tiltDegDelta * gain;
  layer.squashY += clip.squashYDelta * gain;
  layer.stretchX += clip.stretchXDelta * gain;
  layer.roundness += clip.roundnessDelta * gain;
  layer.spacingScale += clip.spacingScaleDelta * gain;
}

FaceService::BlinkType FaceService::chooseBlinkType(const EmotionState& emo) const {
  if (emo.energy < 0.30f) {
    return BlinkType::Sleepy;
  }

  if (emo.attention > 0.76f || emo.arousal > 0.74f || emo.valence > 0.45f) {
    return BlinkType::Attentive;
  }

  const long dice = random(0, 1000);
  const int doubleChance = 16 + static_cast<int>(MathUtils::clamp(emo.curiosity, 0.0f, 1.0f) * 22.0f);
  const int shortChance = 170;

  if (dice < doubleChance) {
    return BlinkType::DoubleRare;
  }
  if (dice < (doubleChance + shortChance)) {
    return BlinkType::Short;
  }
  return BlinkType::Normal;
}

void FaceService::scheduleNextBlink(unsigned long nowMs) {
  const ExpressionPreset& preset = getExpressionPreset(blendToPresetId_);
  const EmotionState& emo = emotionProvider_.getEmotionState();
  const float moodPos = MathUtils::clamp((emo.valence + 1.0f) * 0.5f, 0.0f, 1.0f);

  float intervalScale = 1.0f;
  intervalScale *= 1.0f + ((1.0f - emo.energy) * 0.34f);
  intervalScale *= 1.0f - (emo.attention * 0.18f);
  intervalScale *= 1.0f - (emo.arousal * 0.14f);
  intervalScale *= 1.0f - (moodPos * 0.08f);
  intervalScale = MathUtils::clamp(intervalScale, 0.62f, 1.70f);

  const float jitter = 0.90f + (static_cast<float>(random(0, 211)) / 1000.0f);

  unsigned long minMs = static_cast<unsigned long>(preset.blink.minIntervalMs * intervalScale * jitter);
  unsigned long maxMs = static_cast<unsigned long>(preset.blink.maxIntervalMs * intervalScale * jitter);
  if (maxMs <= minMs) {
    maxMs = minMs + 120;
  }

  nextBlinkAtMs_ = nowMs + random(minMs, maxMs);
}

void FaceService::startBlink(unsigned long nowMs, const FaceLayerState& baseLayer) {
  const ExpressionPreset& preset = getExpressionPreset(blendToPresetId_);
  const EmotionState& emo = emotionProvider_.getEmotionState();
  const float moodPos = MathUtils::clamp((emo.valence + 1.0f) * 0.5f, 0.0f, 1.0f);

  activeBlinkType_ = chooseBlinkType(emo);
  blinkStartMs_ = nowMs;
  blinking_ = true;
  blinkHasSecondPulse_ = false;
  blinkSecondStartMs_ = 0;
  blinkSecondDurationMs_ = 0;

  blinkBaseUpperLid_ = baseLayer.upperLid;
  blinkBaseLowerLid_ = baseLayer.lowerLid;

  const unsigned long baseDuration = preset.blink.durationMs > 0 ? preset.blink.durationMs : 180UL;
  const float jitter = 0.90f + (static_cast<float>(random(0, 211)) / 1000.0f);

  blinkUpperPeak_ = MathUtils::clamp(preset.blink.upperPeak + ((1.0f - emo.energy) * 0.06f) - (moodPos * 0.03f), 0.56f, 0.99f);
  blinkLowerPeak_ = MathUtils::clamp(preset.blink.lowerPeak + ((1.0f - emo.energy) * 0.05f) - (moodPos * 0.02f), 0.12f, 0.80f);

  switch (activeBlinkType_) {
    case BlinkType::Short:
      blinkDurationMs_ = static_cast<unsigned long>(baseDuration * (0.66f - (emo.attention * 0.06f)) * jitter);
      blinkUpperPeak_ = MathUtils::clamp(blinkUpperPeak_ * 0.92f, 0.60f, 0.98f);
      blinkLowerPeak_ = MathUtils::clamp(blinkLowerPeak_ * 0.86f, 0.16f, 0.70f);
      break;

    case BlinkType::DoubleRare:
      blinkDurationMs_ = static_cast<unsigned long>(baseDuration * (0.58f + ((1.0f - emo.energy) * 0.06f)) * jitter);
      blinkHasSecondPulse_ = true;
      blinkSecondDurationMs_ = static_cast<unsigned long>(blinkDurationMs_ * 0.88f);
      blinkSecondStartMs_ = blinkDurationMs_ + static_cast<unsigned long>(random(55, 120));
      blinkUpperPeak_ = MathUtils::clamp(blinkUpperPeak_ * 0.88f, 0.62f, 0.96f);
      blinkLowerPeak_ = MathUtils::clamp(blinkLowerPeak_ * 0.82f, 0.14f, 0.66f);
      break;

    case BlinkType::Sleepy:
      blinkDurationMs_ = static_cast<unsigned long>(baseDuration * (1.36f + ((1.0f - emo.energy) * 0.20f)) * jitter);
      blinkUpperPeak_ = MathUtils::clamp(blinkUpperPeak_ + 0.07f, 0.72f, 0.99f);
      blinkLowerPeak_ = MathUtils::clamp(blinkLowerPeak_ + 0.08f, 0.22f, 0.76f);
      break;

    case BlinkType::Attentive:
      blinkDurationMs_ = static_cast<unsigned long>(baseDuration * (0.72f - (emo.attention * 0.05f)) * jitter);
      blinkUpperPeak_ = MathUtils::clamp(blinkUpperPeak_ * 0.84f, 0.56f, 0.92f);
      blinkLowerPeak_ = MathUtils::clamp(blinkLowerPeak_ * 0.78f, 0.12f, 0.60f);
      break;

    case BlinkType::Normal:
    default:
      blinkDurationMs_ = static_cast<unsigned long>(baseDuration * (1.02f + ((1.0f - emo.energy) * 0.14f) - (moodPos * 0.08f)) * jitter);
      break;
  }

  if (blinkDurationMs_ < 80UL) {
    blinkDurationMs_ = 80UL;
  }

  blinkTotalDurationMs_ = blinkDurationMs_ + 120UL;
  if (blinkHasSecondPulse_) {
    const unsigned long secondEnd = blinkSecondStartMs_ + blinkSecondDurationMs_ + 120UL;
    if (secondEnd > blinkTotalDurationMs_) {
      blinkTotalDurationMs_ = secondEnd;
    }
  }
}

float FaceService::computeSingleBlinkPulse(float t) const {
  if (t <= 0.0f || t >= 1.0f) {
    return 0.0f;
  }

  const float closeEnd = 0.34f;
  const float holdEnd = 0.53f;
  const float openEnd = 0.88f;

  if (t < closeEnd) {
    return easeInOutCubic(t / closeEnd);
  }
  if (t < holdEnd) {
    return 1.0f;
  }
  if (t < openEnd) {
    const float u = (t - holdEnd) / (openEnd - holdEnd);
    return 1.0f - easeInOutCubic(u);
  }

  const float settleT = (t - openEnd) / (1.0f - openEnd);
  const float rebound = 0.09f * sinf(settleT * 3.14159f) * expf(-2.4f * settleT);
  return -rebound;
}

float FaceService::computeBlinkPulse(unsigned long elapsedMs) const {
  if (blinkDurationMs_ == 0) {
    return 0.0f;
  }

  const float primaryT = static_cast<float>(elapsedMs) / static_cast<float>(blinkDurationMs_);
  float pulse = computeSingleBlinkPulse(primaryT);

  if (blinkHasSecondPulse_ && blinkSecondDurationMs_ > 0 && elapsedMs >= blinkSecondStartMs_) {
    const float secondaryT = static_cast<float>(elapsedMs - blinkSecondStartMs_) /
                             static_cast<float>(blinkSecondDurationMs_);
    const float secondPulse = computeSingleBlinkPulse(secondaryT);
    if (secondPulse > pulse) {
      pulse = secondPulse;
    }
  }

  return MathUtils::clamp(pulse, -0.20f, 1.0f);
}

void FaceService::applyBlinkLayer(FaceLayerState& layer, unsigned long nowMs, const EmotionState&) {
  if (!blinking_ && nowMs >= nextBlinkAtMs_) {
    startBlink(nowMs, layer);
  }

  if (!blinking_) {
    return;
  }

  const unsigned long elapsed = nowMs - blinkStartMs_;
  if (blinkTotalDurationMs_ == 0 || elapsed >= blinkTotalDurationMs_) {
    blinking_ = false;
    scheduleNextBlink(nowMs);
    return;
  }

  const float pulse = computeBlinkPulse(elapsed);
  const float blinkGain = MathUtils::clamp(facePresetLayerBlend(blendToPresetId_).blinkGain, 0.60f, 1.20f);
  const float adjustedUpperPeak = lerpF(blinkBaseUpperLid_, blinkUpperPeak_, blinkGain);
  const float adjustedLowerPeak = lerpF(blinkBaseLowerLid_, blinkLowerPeak_, blinkGain);
  layer.upperLid = lerpF(blinkBaseUpperLid_, adjustedUpperPeak, pulse);
  layer.lowerLid = lerpF(blinkBaseLowerLid_, adjustedLowerPeak, pulse);
}

FaceService::FaceLayerDelta FaceService::captureDelta(const FaceLayerState& before,
                                                      const FaceLayerState& after) const {
  FaceLayerDelta delta;
  delta.leftX = after.leftX - before.leftX;
  delta.rightX = after.rightX - before.rightX;
  delta.y = after.y - before.y;
  delta.openness = after.openness - before.openness;
  delta.upperLid = after.upperLid - before.upperLid;
  delta.lowerLid = after.lowerLid - before.lowerLid;
  delta.tiltDeg = after.tiltDeg - before.tiltDeg;
  delta.squashY = after.squashY - before.squashY;
  delta.stretchX = after.stretchX - before.stretchX;
  delta.roundness = after.roundness - before.roundness;
  delta.spacingScale = after.spacingScale - before.spacingScale;
  delta.yawNorm = after.yawNorm - before.yawNorm;
  delta.tiltNorm = after.tiltNorm - before.tiltNorm;
  return delta;
}

void FaceService::applyDelta(FaceLayerState& layer, const FaceLayerDelta& delta) const {
  layer.leftX += delta.leftX;
  layer.rightX += delta.rightX;
  layer.y += delta.y;
  layer.openness += delta.openness;
  layer.upperLid += delta.upperLid;
  layer.lowerLid += delta.lowerLid;
  layer.tiltDeg += delta.tiltDeg;
  layer.squashY += delta.squashY;
  layer.stretchX += delta.stretchX;
  layer.roundness += delta.roundness;
  layer.spacingScale += delta.spacingScale;
  layer.yawNorm += delta.yawNorm;
  layer.tiltNorm += delta.tiltNorm;
}

FaceService::FaceLayerState FaceService::resolveLayerWithCompositor(const FaceLayerState& baseLayer,
                                                                    const FaceLayerDelta& gazeDelta,
                                                                    const FaceLayerDelta& idleDelta,
                                                                    const FaceLayerDelta& transientDelta,
                                                                    const FaceLayerDelta& clipDelta,
                                                                    const FaceLayerDelta& blinkDelta) const {
  FaceLayerState resolved = baseLayer;
  if (renderState_.gaze.enabled) {
    applyDelta(resolved, gazeDelta);
  }

  if (renderState_.idle.enabled) {
    applyDelta(resolved, idleDelta);
  }

  if (renderState_.transient.active) {
    applyDelta(resolved, transientDelta);
  }

  if (renderState_.clip.active) {
    applyDelta(resolved, clipDelta);
  }

  if (renderState_.blink.active) {
    applyDelta(resolved, blinkDelta);
  }

  return resolved;
}

void FaceService::commitLayerState(const FaceLayerState& layerIn) {
  FaceLayerState layer = layerIn;

  layer.spacingScale = MathUtils::clamp(layer.spacingScale, 0.92f, 1.10f);
  layer.stretchX = MathUtils::clamp(layer.stretchX, 0.82f, 1.40f);
  layer.squashY = MathUtils::clamp(layer.squashY, 0.62f, 1.30f);
  layer.tiltDeg = MathUtils::clamp(layer.tiltDeg, -26.0f, 26.0f);
  layer.roundness = MathUtils::clamp(layer.roundness, 0.22f, 0.88f);
  layer.upperLid = MathUtils::clamp(layer.upperLid, 0.0f, 0.95f);
  layer.lowerLid = MathUtils::clamp(layer.lowerLid, 0.0f, 0.82f);
  layer.openness = MathUtils::clamp(layer.openness, 0.38f, 1.12f);

  targetLeftEye_.centerX = layer.leftX;
  targetRightEye_.centerX = layer.rightX;
  targetLeftEye_.centerY = layer.y;
  targetRightEye_.centerY = layer.y;

  targetLeftEye_.expression = currentExpression_;
  targetRightEye_.expression = currentExpression_;

  targetLeftEye_.tiltDeg = -layer.tiltDeg;
  targetRightEye_.tiltDeg = layer.tiltDeg;

  targetLeftEye_.squashY = layer.squashY;
  targetRightEye_.squashY = layer.squashY;

  targetLeftEye_.stretchX = layer.stretchX;
  targetRightEye_.stretchX = layer.stretchX;

  targetLeftEye_.upperLid = layer.upperLid;
  targetRightEye_.upperLid = layer.upperLid;
  targetLeftEye_.lowerLid = layer.lowerLid;
  targetRightEye_.lowerLid = layer.lowerLid;

  targetLeftEye_.roundness = layer.roundness;
  targetRightEye_.roundness = layer.roundness;

  const bool geometryV2Enabled = HardwareConfig::Face::GEOMETRY_V2_ENABLED;
  targetLeftEye_.useShapeV2 = geometryV2Enabled;
  targetRightEye_.useShapeV2 = geometryV2Enabled;
  if (geometryV2Enabled) {
    const EmotionState& emo = emotionProvider_.getEmotionState();
    const FaceShapeProfile shapeProfile = FaceGeometry::profileForExpression(currentExpression_, emo, layer.roundness);
    targetLeftEye_.shape = FaceGeometry::resolveEyeShape(shapeProfile, EyeSide::Left);
    targetRightEye_.shape = FaceGeometry::resolveEyeShape(shapeProfile, EyeSide::Right);
  }

  baseOpenness_ = layer.openness;
  targetLeftEye_.openness = baseOpenness_;
  targetRightEye_.openness = baseOpenness_;
}
void FaceService::smoothEyes(unsigned long nowMs) {
  const unsigned long dtMs = nowMs - lastSmoothMs_;
  lastSmoothMs_ = nowMs;

  const ExpressionPreset& fromPreset = getExpressionPreset(blendFromPresetId_);
  const ExpressionPreset& toPreset = getExpressionPreset(blendToPresetId_);
  const float blend = computePresetBlend(nowMs);

  const EmotionState& emo = emotionProvider_.getEmotionState();
  const float moodPos = MathUtils::clamp((emo.valence + 1.0f) * 0.5f, 0.0f, 1.0f);

  const float dt = MathUtils::clamp(static_cast<float>(dtMs) / 1000.0f, 0.0f, 0.050f);
  float responseHz = lerpF(fromPreset.transition.responseHz, toPreset.transition.responseHz, blend);
  float alphaMin = lerpF(fromPreset.transition.alphaMin, toPreset.transition.alphaMin, blend);
  float alphaMax = lerpF(fromPreset.transition.alphaMax, toPreset.transition.alphaMax, blend);

  responseHz *= 0.86f + (emo.attention * 0.28f) + (emo.arousal * 0.18f);
  responseHz *= 0.84f + (emo.energy * 0.24f);
  alphaMax *= 0.88f + (moodPos * 0.16f);
  alphaMin *= 0.92f + (emo.energy * 0.08f);

  responseHz = MathUtils::clamp(responseHz, 5.5f, 22.0f);
  alphaMin = MathUtils::clamp(alphaMin, 0.025f, 0.24f);
  alphaMax = MathUtils::clamp(alphaMax, 0.08f, 0.62f);

  if (nowMs < presetSettleUntilMs_) {
    alphaMax *= 0.74f;
  }

#if NCOS_SIM_MODE
  alphaMax *= 0.78f;
#endif

  float alpha = 1.0f - expf(-(responseHz) * dt);
  alpha = MathUtils::clamp(alpha, alphaMin, alphaMax);

  leftEye_.openness = lerpF(leftEye_.openness, targetLeftEye_.openness, alpha);
  rightEye_.openness = lerpF(rightEye_.openness, targetRightEye_.openness, alpha);

  leftEye_.upperLid = lerpF(leftEye_.upperLid, targetLeftEye_.upperLid, alpha);
  rightEye_.upperLid = lerpF(rightEye_.upperLid, targetRightEye_.upperLid, alpha);

  leftEye_.lowerLid = lerpF(leftEye_.lowerLid, targetLeftEye_.lowerLid, alpha);
  rightEye_.lowerLid = lerpF(rightEye_.lowerLid, targetRightEye_.lowerLid, alpha);

  leftEye_.tiltDeg = lerpF(leftEye_.tiltDeg, targetLeftEye_.tiltDeg, alpha);
  rightEye_.tiltDeg = lerpF(rightEye_.tiltDeg, targetRightEye_.tiltDeg, alpha);

  leftEye_.squashY = lerpF(leftEye_.squashY, targetLeftEye_.squashY, alpha);
  rightEye_.squashY = lerpF(rightEye_.squashY, targetRightEye_.squashY, alpha);

  leftEye_.stretchX = lerpF(leftEye_.stretchX, targetLeftEye_.stretchX, alpha);
  rightEye_.stretchX = lerpF(rightEye_.stretchX, targetRightEye_.stretchX, alpha);

  leftEye_.roundness = lerpF(leftEye_.roundness, targetLeftEye_.roundness, alpha);
  rightEye_.roundness = lerpF(rightEye_.roundness, targetRightEye_.roundness, alpha);

  leftEye_.centerX = static_cast<int>(lerpF(static_cast<float>(leftEye_.centerX), static_cast<float>(targetLeftEye_.centerX), alpha));
  rightEye_.centerX = static_cast<int>(lerpF(static_cast<float>(rightEye_.centerX), static_cast<float>(targetRightEye_.centerX), alpha));
  leftEye_.centerY = static_cast<int>(lerpF(static_cast<float>(leftEye_.centerY), static_cast<float>(targetLeftEye_.centerY), alpha));
  rightEye_.centerY = static_cast<int>(lerpF(static_cast<float>(rightEye_.centerY), static_cast<float>(targetRightEye_.centerY), alpha));

  leftEye_.expression = targetLeftEye_.expression;
  rightEye_.expression = targetRightEye_.expression;
  leftEye_.useShapeV2 = targetLeftEye_.useShapeV2;
  rightEye_.useShapeV2 = targetRightEye_.useShapeV2;
  leftEye_.shape = targetLeftEye_.shape;
  rightEye_.shape = targetRightEye_.shape;
}

float FaceService::easeInOutCubic(float t) const {
  if (t < 0.5f) {
    return 4.0f * t * t * t;
  }
  const float k = (-2.0f * t) + 2.0f;
  return 1.0f - ((k * k * k) / 2.0f);
}

void FaceService::recordPerf(unsigned long nowMs, bool rendered, unsigned long renderUs) {
  perf_.updates++;

  if (rendered) {
    perf_.renderedFrames++;
    perf_.avgRenderUsAccum += renderUs;
    if (renderUs > perf_.maxRenderUs) {
      perf_.maxRenderUs = renderUs;
    }
  } else {
    perf_.droppedFrames++;
  }

  if (nowMs - perf_.windowStartMs < kPerfWindowMs) {
    return;
  }

  perf_.windowStartMs = nowMs;
  perf_.updates = 0;
  perf_.renderedFrames = 0;
  perf_.droppedFrames = 0;
  perf_.maxRenderUs = 0;
  perf_.avgRenderUsAccum = 0;
}







































