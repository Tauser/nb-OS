#include "emotion_service.h"

#include "../../config/hardware_config.h"
#include "../../utils/math_utils.h"
#include <Arduino.h>
#include <math.h>

namespace {
float approach(float current, float target, float alpha) {
  return current + ((target - current) * alpha);
}

float absfLocal(float value) {
  return fabsf(value);
}

float scoreForMode(const HomeostasisScores& s, HomeostasisMode mode) {
  switch (mode) {
    case HomeostasisMode::Calm: return s.calm;
    case HomeostasisMode::Animated: return s.animated;
    case HomeostasisMode::Curious: return s.curious;
    case HomeostasisMode::Social: return s.social;
    case HomeostasisMode::Sensitive: return s.sensitive;
    case HomeostasisMode::Sleepy: return s.sleepy;
    case HomeostasisMode::Bored: return s.bored;
    default: return 0.0f;
  }
}
}

EmotionService::EmotionService(EventBus& eventBus)
    : eventBus_(eventBus) {}

void EmotionService::init() {
  reset();
  eventBus_.subscribe(EventType::Any, this);
  const unsigned long nowMs = millis();
  lastUpdateMs_ = nowMs;
  lastInteractionMs_ = nowMs;
  modeSinceMs_ = nowMs;
}

void EmotionService::update(unsigned long nowMs) {
  if (lastUpdateMs_ == 0 || nowMs <= lastUpdateMs_) {
    lastUpdateMs_ = nowMs;
    return;
  }

  const float dtS = static_cast<float>(nowMs - lastUpdateMs_) / 1000.0f;
  lastUpdateMs_ = nowMs;

  homeo_.idleMs = nowMs - lastInteractionMs_;
  applyHomeostaticDecay(dtS);
  applyAdaptiveDecay(dtS);
  computeEmotionalScores(nowMs);
  synthesizeEmotionFromHomeostasis();

  if (shouldPublish(nowMs)) {
    publishEmotionChanged(nowMs);
  }
}

void EmotionService::onEvent(const Event& event) {
  if (event.type == EventType::EVT_EMOTION_CHANGED || event.source == EventSource::EmotionService) {
    return;
  }

  const unsigned long nowMs = event.timestamp > 0 ? event.timestamp : millis();
  lastObservedEventType_ = event.type;

  if (event.type == EventType::EVT_TOUCH ||
      event.type == EventType::EVT_VOICE_START ||
      event.type == EventType::EVT_VOICE_ACTIVITY ||
      event.type == EventType::EVT_TILT ||
      event.type == EventType::EVT_SHAKE ||
      event.type == EventType::EVT_INTENT_DETECTED) {
    lastInteractionMs_ = nowMs;
  }

  applyEventRule(event);
  homeo_.idleMs = nowMs - lastInteractionMs_;
  computeEmotionalScores(nowMs);
  synthesizeEmotionFromHomeostasis();

  if (shouldPublish(nowMs)) {
    publishEmotionChanged(nowMs);
  }
}

const EmotionState& EmotionService::getEmotionState() const {
  return state_;
}

const HomeostasisState& EmotionService::getHomeostasisState() const {
  return homeo_;
}

void EmotionService::reset() {
  baseline_ = EmotionState::neutral();
  state_ = EmotionState::neutral();
  lastPublishedState_ = state_;

  homeo_ = HomeostasisState{};
  homeo_.mode = HomeostasisMode::Calm;
  homeo_.clamp();
  lastPublishedHomeo_ = homeo_;

  lastObservedEventType_ = EventType::None;
  changedSincePublish_ = false;
  lastPublishMs_ = millis();
}

void EmotionService::applyEventRule(const Event& event) {
  switch (event.type) {
    case EventType::EVT_TOUCH:
      applyHomeoDelta(HardwareConfig::Homeostasis::TOUCH_ENERGY,
                      HardwareConfig::Homeostasis::TOUCH_STIMULATION,
                      HardwareConfig::Homeostasis::TOUCH_CURIOSITY);
      homeo_.sociabilityBias += HardwareConfig::Homeostasis::BIAS_LEARN_PER_EVENT;
      homeo_.affinityBias += HardwareConfig::Homeostasis::BIAS_LEARN_PER_EVENT;
      homeo_.sensitivityBias -= (HardwareConfig::Homeostasis::BIAS_LEARN_PER_EVENT * 0.35f);
      break;

    case EventType::EVT_SHAKE:
      applyHomeoDelta(HardwareConfig::Homeostasis::SHAKE_ENERGY,
                      HardwareConfig::Homeostasis::SHAKE_STIMULATION,
                      0.0f);
      homeo_.sensitivityBias += (HardwareConfig::Homeostasis::BIAS_LEARN_PER_EVENT * 1.1f);
      homeo_.sociabilityBias -= (HardwareConfig::Homeostasis::BIAS_LEARN_PER_EVENT * 0.45f);
      break;

    case EventType::EVT_TILT:
      applyHomeoDelta(0.0f,
                      HardwareConfig::Homeostasis::TILT_STIMULATION,
                      HardwareConfig::Homeostasis::TILT_CURIOSITY);
      homeo_.noveltyBias += (HardwareConfig::Homeostasis::BIAS_LEARN_PER_EVENT * 0.9f);
      break;

    case EventType::EVT_FALL:
      applyHomeoDelta(HardwareConfig::Homeostasis::FALL_ENERGY,
                      HardwareConfig::Homeostasis::FALL_STIMULATION,
                      0.0f);
      homeo_.sensitivityBias += (HardwareConfig::Homeostasis::BIAS_LEARN_PER_EVENT * 1.4f);
      homeo_.affinityBias -= (HardwareConfig::Homeostasis::BIAS_LEARN_PER_EVENT * 0.5f);
      break;

    case EventType::EVT_IDLE:
      applyHomeoDelta(0.0f,
                      HardwareConfig::Homeostasis::IDLE_STIMULATION,
                      HardwareConfig::Homeostasis::IDLE_CURIOSITY);
      break;

    case EventType::EVT_VOICE_START:
    case EventType::EVT_VOICE_ACTIVITY:
      applyHomeoDelta(0.0f,
                      HardwareConfig::Homeostasis::VOICE_STIMULATION,
                      HardwareConfig::Homeostasis::VOICE_CURIOSITY);
      homeo_.sociabilityBias += (HardwareConfig::Homeostasis::BIAS_LEARN_PER_EVENT * 0.8f);
      homeo_.affinityBias += (HardwareConfig::Homeostasis::BIAS_LEARN_PER_EVENT * 0.55f);
      break;

    case EventType::EVT_INTENT_DETECTED:
      applyHomeoDelta(0.01f,
                      HardwareConfig::Homeostasis::VOICE_STIMULATION * 0.75f,
                      HardwareConfig::Homeostasis::VOICE_CURIOSITY * 0.75f);
      homeo_.noveltyBias += (HardwareConfig::Homeostasis::BIAS_LEARN_PER_EVENT * 0.5f);
      break;

    default:
      break;
  }

  homeo_.clamp();
}

void EmotionService::applyHomeostaticDecay(float dtS) {
  if (dtS <= 0.0f) {
    return;
  }

  const float aEnergy = MathUtils::clamp(HardwareConfig::Homeostasis::ENERGY_DECAY_PER_S * dtS, 0.0f, 1.0f);
  const float aStim = MathUtils::clamp(HardwareConfig::Homeostasis::STIMULATION_DECAY_PER_S * dtS, 0.0f, 1.0f);
  const float aCur = MathUtils::clamp(HardwareConfig::Homeostasis::CURIOSITY_DECAY_PER_S * dtS, 0.0f, 1.0f);

  homeo_.energy = approach(homeo_.energy, HardwareConfig::Homeostasis::ENERGY_BASELINE, aEnergy);
  homeo_.stimulation = approach(homeo_.stimulation, HardwareConfig::Homeostasis::STIMULATION_BASELINE, aStim);
  homeo_.curiosity = approach(homeo_.curiosity, HardwareConfig::Homeostasis::CURIOSITY_BASELINE, aCur);

  if (homeo_.idleMs >= HardwareConfig::Homeostasis::LONG_IDLE_MS) {
    const float idleDT = dtS;
    homeo_.stimulation = MathUtils::clamp(homeo_.stimulation - (0.03f * idleDT), 0.0f, 1.0f);
    homeo_.energy = MathUtils::clamp(homeo_.energy - (0.02f * idleDT), 0.0f, 1.0f);
  }

  homeo_.clamp();
}

void EmotionService::applyAdaptiveDecay(float dtS) {
  if (dtS <= 0.0f) {
    return;
  }

  const float alpha = MathUtils::clamp(HardwareConfig::Homeostasis::BIAS_DECAY_PER_S * dtS, 0.0f, 1.0f);
  homeo_.sociabilityBias = approach(homeo_.sociabilityBias, 0.45f, alpha);
  homeo_.sensitivityBias = approach(homeo_.sensitivityBias, 0.35f, alpha);
  homeo_.affinityBias = approach(homeo_.affinityBias, 0.40f, alpha);
  homeo_.noveltyBias = approach(homeo_.noveltyBias, 0.45f, alpha);
}

void EmotionService::computeEmotionalScores(unsigned long nowMs) {
  (void)nowMs;
  const float e = homeo_.energy;
  const float s = homeo_.stimulation;
  const float c = homeo_.curiosity;
  const float soc = homeo_.sociabilityBias;
  const float sen = homeo_.sensitivityBias;
  const float aff = homeo_.affinityBias;
  const float nov = homeo_.noveltyBias;

  const float idleLong = MathUtils::clamp(static_cast<float>(homeo_.idleMs) / static_cast<float>(HardwareConfig::Homeostasis::LONG_IDLE_MS), 0.0f, 1.0f);
  const float idleBored = MathUtils::clamp(static_cast<float>(homeo_.idleMs) / static_cast<float>(HardwareConfig::Homeostasis::BORED_IDLE_MS), 0.0f, 1.0f);

  homeo_.scores.sleepy = ((1.0f - e) * 0.55f) + (idleLong * 0.25f) + ((1.0f - s) * 0.20f);
  homeo_.scores.bored = (idleBored * 0.55f) + ((1.0f - s) * 0.30f) + ((1.0f - c) * 0.15f);
  homeo_.scores.curious = (c * 0.60f) + (nov * 0.25f) + (s * 0.15f);
  homeo_.scores.social = (soc * 0.50f) + (aff * 0.30f) + (s * 0.20f);
  homeo_.scores.sensitive = (sen * 0.60f) + (s * 0.25f) + ((1.0f - e) * 0.15f);
  homeo_.scores.animated = (s * 0.55f) + (e * 0.30f) + (nov * 0.15f);

  const float calmRaw = ((1.0f - s) * 0.46f) + (e * 0.24f) + ((1.0f - homeo_.scores.sensitive) * 0.30f);
  homeo_.scores.calm = MathUtils::clamp(calmRaw, 0.0f, 1.0f);
  homeo_.scores.clamp();

  const HomeostasisMode newMode = dominantModeWithHysteresis(lastUpdateMs_);
  if (newMode != homeo_.mode) {
    homeo_.mode = newMode;
    modeSinceMs_ = lastUpdateMs_;
  }
}

void EmotionService::synthesizeEmotionFromHomeostasis() {
  const EmotionState before = state_;

  state_.energy = homeo_.energy;
  state_.curiosity = MathUtils::clamp((homeo_.curiosity * 0.65f) + (homeo_.noveltyBias * 0.20f) + (homeo_.scores.curious * 0.15f), 0.0f, 1.0f);
  state_.attention = MathUtils::clamp((homeo_.stimulation * 0.45f) + (homeo_.scores.curious * 0.30f) + (homeo_.scores.social * 0.25f), 0.0f, 1.0f);
  state_.bond = MathUtils::clamp((homeo_.affinityBias * 0.55f) + (homeo_.scores.social * 0.25f) + (homeo_.sociabilityBias * 0.20f), 0.0f, 1.0f);

  state_.arousal = MathUtils::clamp((homeo_.stimulation * 0.52f) + (homeo_.scores.animated * 0.30f) + (homeo_.scores.sensitive * 0.18f), 0.0f, 1.0f);

  const float positive = (homeo_.scores.social * 0.45f) + (homeo_.scores.calm * 0.35f) + (homeo_.scores.curious * 0.20f);
  const float negative = (homeo_.scores.sensitive * 0.45f) + (homeo_.scores.bored * 0.30f) + (homeo_.scores.sleepy * 0.25f);
  state_.valence = MathUtils::clamp(positive - negative, EmotionState::kValenceMin, EmotionState::kValenceMax);

  switch (homeo_.mode) {
    case HomeostasisMode::Sleepy:
      state_.energy = MathUtils::clamp(state_.energy - 0.10f, 0.0f, 1.0f);
      state_.arousal = MathUtils::clamp(state_.arousal - 0.08f, 0.0f, 1.0f);
      break;
    case HomeostasisMode::Bored:
      state_.attention = MathUtils::clamp(state_.attention - 0.10f, 0.0f, 1.0f);
      state_.valence = MathUtils::clamp(state_.valence - 0.08f, EmotionState::kValenceMin, EmotionState::kValenceMax);
      break;
    case HomeostasisMode::Animated:
      state_.arousal = MathUtils::clamp(state_.arousal + 0.08f, 0.0f, 1.0f);
      state_.attention = MathUtils::clamp(state_.attention + 0.06f, 0.0f, 1.0f);
      break;
    case HomeostasisMode::Social:
      state_.bond = MathUtils::clamp(state_.bond + 0.08f, 0.0f, 1.0f);
      state_.valence = MathUtils::clamp(state_.valence + 0.08f, EmotionState::kValenceMin, EmotionState::kValenceMax);
      break;
    case HomeostasisMode::Sensitive:
      state_.arousal = MathUtils::clamp(state_.arousal + 0.05f, 0.0f, 1.0f);
      state_.valence = MathUtils::clamp(state_.valence - 0.06f, EmotionState::kValenceMin, EmotionState::kValenceMax);
      break;
    case HomeostasisMode::Curious:
      state_.curiosity = MathUtils::clamp(state_.curiosity + 0.06f, 0.0f, 1.0f);
      break;
    case HomeostasisMode::Calm:
    default:
      break;
  }

  state_.normalize();

  if (stateDelta(state_, before) > 0.0001f || homeo_.mode != lastPublishedHomeo_.mode) {
    changedSincePublish_ = true;
  }
}

HomeostasisMode EmotionService::dominantModeWithHysteresis(unsigned long nowMs) const {
  const HomeostasisMode current = homeo_.mode;

  HomeostasisMode candidate = HomeostasisMode::Calm;
  float best = homeo_.scores.calm;

  const HomeostasisMode modes[] = {
      HomeostasisMode::Animated,
      HomeostasisMode::Curious,
      HomeostasisMode::Social,
      HomeostasisMode::Sensitive,
      HomeostasisMode::Sleepy,
      HomeostasisMode::Bored};

  for (HomeostasisMode mode : modes) {
    const float score = scoreForMode(homeo_.scores, mode);
    if (score > best) {
      best = score;
      candidate = mode;
    }
  }

  const float currentScore = scoreForMode(homeo_.scores, current);
  if (candidate == current) {
    return current;
  }

  if (best < (currentScore + HardwareConfig::Homeostasis::HYSTERESIS_MARGIN)) {
    return current;
  }

  if (nowMs - modeSinceMs_ < HardwareConfig::Homeostasis::HYSTERESIS_HOLD_MS) {
    return current;
  }

  return candidate;
}

void EmotionService::applyHomeoDelta(float dEnergy, float dStimulation, float dCuriosity) {
  homeo_.energy += dEnergy;
  homeo_.stimulation += dStimulation;
  homeo_.curiosity += dCuriosity;
  homeo_.clamp();
  changedSincePublish_ = true;
}

bool EmotionService::shouldPublish(unsigned long nowMs) const {
  if (!changedSincePublish_) {
    return false;
  }

  if (nowMs - lastPublishMs_ < HardwareConfig::Emotion::CHANGE_PUBLISH_MIN_INTERVAL_MS) {
    return false;
  }

  const float delta = stateDelta(state_, lastPublishedState_);
  const bool modeChanged = (homeo_.mode != lastPublishedHomeo_.mode);
  return modeChanged || delta >= HardwareConfig::Emotion::CHANGE_PUBLISH_THRESHOLD;
}

void EmotionService::publishEmotionChanged(unsigned long nowMs) {
  Event event;
  event.type = EventType::EVT_EMOTION_CHANGED;
  event.source = EventSource::EmotionService;
  event.value = static_cast<int>(state_.arousal * 1000.0f);
  event.timestamp = nowMs;
  eventBus_.publish(event);

  lastPublishedState_ = state_;
  lastPublishedHomeo_ = homeo_;
  lastPublishMs_ = nowMs;
  changedSincePublish_ = false;
}

float EmotionService::stateDelta(const EmotionState& a, const EmotionState& b) const {
  float sum = 0.0f;
  sum += absfLocal(a.valence - b.valence);
  sum += absfLocal(a.arousal - b.arousal);
  sum += absfLocal(a.curiosity - b.curiosity);
  sum += absfLocal(a.attention - b.attention);
  sum += absfLocal(a.bond - b.bond);
  sum += absfLocal(a.energy - b.energy);
  return sum;
}
