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
}

EmotionService::EmotionService(EventBus& eventBus)
    : eventBus_(eventBus) {}

void EmotionService::init() {
  reset();
  eventBus_.subscribe(EventType::Any, this);
  lastUpdateMs_ = millis();
}

void EmotionService::update(unsigned long nowMs) {
  if (lastUpdateMs_ == 0 || nowMs <= lastUpdateMs_) {
    lastUpdateMs_ = nowMs;
    return;
  }

  const float dtS = static_cast<float>(nowMs - lastUpdateMs_) / 1000.0f;
  lastUpdateMs_ = nowMs;

  applyDecay(dtS);

  if (shouldPublish(nowMs)) {
    publishEmotionChanged(nowMs);
  }
}

void EmotionService::onEvent(const Event& event) {
  if (event.type == EventType::EVT_EMOTION_CHANGED || event.source == EventSource::EmotionService) {
    return;
  }

  lastObservedEventType_ = event.type;
  applyEventRule(event);

  const unsigned long nowMs = event.timestamp > 0 ? event.timestamp : millis();
  if (shouldPublish(nowMs)) {
    publishEmotionChanged(nowMs);
  }
}

const EmotionState& EmotionService::getEmotionState() const {
  return state_;
}

void EmotionService::reset() {
  baseline_ = EmotionState::neutral();
  state_ = EmotionState::neutral();
  lastPublishedState_ = state_;
  lastObservedEventType_ = EventType::None;
  changedSincePublish_ = false;
  lastPublishMs_ = millis();
}

void EmotionService::applyEventRule(const Event& event) {
  switch (event.type) {
    case EventType::EVT_TOUCH:
      applyDelta(HardwareConfig::Emotion::TOUCH_D_VALENCE,
                 HardwareConfig::Emotion::TOUCH_D_AROUSAL,
                 HardwareConfig::Emotion::TOUCH_D_CURIOSITY,
                 HardwareConfig::Emotion::TOUCH_D_ATTENTION,
                 HardwareConfig::Emotion::TOUCH_D_BOND,
                 0.0f);
      break;

    case EventType::EVT_SHAKE:
      applyDelta(HardwareConfig::Emotion::SHAKE_D_VALENCE,
                 HardwareConfig::Emotion::SHAKE_D_AROUSAL,
                 0.0f,
                 HardwareConfig::Emotion::SHAKE_D_ATTENTION,
                 0.0f,
                 0.0f);
      break;

    case EventType::EVT_TILT:
      applyDelta(0.0f,
                 0.0f,
                 HardwareConfig::Emotion::TILT_D_CURIOSITY,
                 HardwareConfig::Emotion::TILT_D_ATTENTION,
                 0.0f,
                 0.0f);
      break;

    case EventType::EVT_FALL:
      applyDelta(HardwareConfig::Emotion::FALL_D_VALENCE,
                 HardwareConfig::Emotion::FALL_D_AROUSAL,
                 0.0f,
                 0.0f,
                 0.0f,
                 HardwareConfig::Emotion::FALL_D_ENERGY);
      break;

    case EventType::EVT_IDLE:
      applyDelta(0.0f,
                 HardwareConfig::Emotion::IDLE_D_AROUSAL,
                 HardwareConfig::Emotion::IDLE_D_CURIOSITY,
                 HardwareConfig::Emotion::IDLE_D_ATTENTION,
                 0.0f,
                 0.0f);
      break;

    case EventType::EVT_VOICE_ACTIVITY:
      applyDelta(0.0f,
                 HardwareConfig::Emotion::VOICE_D_AROUSAL,
                 0.0f,
                 HardwareConfig::Emotion::VOICE_D_ATTENTION,
                 HardwareConfig::Emotion::VOICE_D_BOND,
                 0.0f);
      break;

    default:
      break;
  }
}

void EmotionService::applyDecay(float dtS) {
  if (dtS <= 0.0f) {
    return;
  }

  const float aValence = MathUtils::clamp(HardwareConfig::Emotion::DECAY_VALENCE_PER_S * dtS, 0.0f, 1.0f);
  const float aArousal = MathUtils::clamp(HardwareConfig::Emotion::DECAY_AROUSAL_PER_S * dtS, 0.0f, 1.0f);
  const float aCuriosity = MathUtils::clamp(HardwareConfig::Emotion::DECAY_CURIOSITY_PER_S * dtS, 0.0f, 1.0f);
  const float aAttention = MathUtils::clamp(HardwareConfig::Emotion::DECAY_ATTENTION_PER_S * dtS, 0.0f, 1.0f);
  const float aBond = MathUtils::clamp(HardwareConfig::Emotion::DECAY_BOND_PER_S * dtS, 0.0f, 1.0f);
  const float aEnergy = MathUtils::clamp(HardwareConfig::Emotion::DECAY_ENERGY_PER_S * dtS, 0.0f, 1.0f);

  const EmotionState before = state_;

  state_.valence = approach(state_.valence, baseline_.valence, aValence);
  state_.arousal = approach(state_.arousal, baseline_.arousal, aArousal);
  state_.curiosity = approach(state_.curiosity, baseline_.curiosity, aCuriosity);
  state_.attention = approach(state_.attention, baseline_.attention, aAttention);
  state_.bond = approach(state_.bond, baseline_.bond, aBond);
  state_.energy = approach(state_.energy, baseline_.energy, aEnergy);
  state_.normalize();

  if (stateDelta(state_, before) > 0.0001f) {
    changedSincePublish_ = true;
  }
}

void EmotionService::applyDelta(float dValence,
                                float dArousal,
                                float dCuriosity,
                                float dAttention,
                                float dBond,
                                float dEnergy) {
  state_.valence += dValence;
  state_.arousal += dArousal;
  state_.curiosity += dCuriosity;
  state_.attention += dAttention;
  state_.bond += dBond;
  state_.energy += dEnergy;
  state_.normalize();
  changedSincePublish_ = true;
}

bool EmotionService::shouldPublish(unsigned long nowMs) const {
  if (!changedSincePublish_) {
    return false;
  }

  if (nowMs - lastPublishMs_ < HardwareConfig::Emotion::CHANGE_PUBLISH_MIN_INTERVAL_MS) {
    return false;
  }

  return stateDelta(state_, lastPublishedState_) >= HardwareConfig::Emotion::CHANGE_PUBLISH_THRESHOLD;
}

void EmotionService::publishEmotionChanged(unsigned long nowMs) {
  Event event;
  event.type = EventType::EVT_EMOTION_CHANGED;
  event.source = EventSource::EmotionService;
  event.value = static_cast<int>(state_.arousal * 1000.0f);
  event.timestamp = nowMs;
  eventBus_.publish(event);

  lastPublishedState_ = state_;
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
