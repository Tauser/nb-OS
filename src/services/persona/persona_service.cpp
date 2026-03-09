#include "persona_service.h"

#include "../../config/hardware_config.h"
#include "../../models/attention_focus.h"
#include "../../models/event.h"
#include "../../utils/math_utils.h"
#include <Arduino.h>
#include <math.h>

namespace {
float approach(float current, float target, float ratePerS, float dtS) {
  const float alpha = MathUtils::clamp(ratePerS * dtS, 0.0f, 1.0f);
  return current + ((target - current) * alpha);
}
}

PersonaService::PersonaService(EventBus& eventBus)
    : eventBus_(eventBus) {}

void PersonaService::init() {
  eventBus_.subscribe(EventType::EVT_MOOD_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_ENGAGEMENT_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_AFFINITY_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_PREFERENCE_UPDATED, this);
  eventBus_.subscribe(EventType::EVT_MEMORY_UPDATED, this);
  eventBus_.subscribe(EventType::EVT_VOICE_START, this);
  eventBus_.subscribe(EventType::EVT_VOICE_ACTIVITY, this);
  eventBus_.subscribe(EventType::EVT_TOUCH, this);

  const unsigned long nowMs = millis();
  lastUpdateMs_ = nowMs;
  lastInteractionMs_ = nowMs;

  profile_.baseTone = PersonaTone::Warm;
  profile_.tone = profile_.baseTone;
  profile_.responseStyle = PersonaResponseStyle::Balanced;
  profile_.clamp();

  publish(nowMs);
}

void PersonaService::update(unsigned long nowMs) {
  if (nowMs - lastUpdateMs_ < HardwareConfig::Companion::PERSONA_UPDATE_INTERVAL_MS) {
    return;
  }

  const float dtS = static_cast<float>(nowMs - lastUpdateMs_) / 1000.0f;
  lastUpdateMs_ = nowMs;

  const float idleFactor = MathUtils::clamp(static_cast<float>(nowMs - lastInteractionMs_) / static_cast<float>(HardwareConfig::Homeostasis::LONG_IDLE_MS), 0.0f, 1.0f);

  if (prefersCalm_) {
    profile_.baseTone = PersonaTone::Calm;
  } else if (voicePreference_ > HardwareConfig::Companion::PERSONA_VOICE_PREF_PLAYFUL_MIN) {
    profile_.baseTone = PersonaTone::Playful;
  } else {
    profile_.baseTone = PersonaTone::Warm;
  }

  PersonaTone targetTone = profile_.baseTone;
  if (mood_ < HardwareConfig::Companion::PERSONA_NEG_MOOD_CALM_MAX || engagement_ < HardwareConfig::Companion::PERSONA_LOW_ENGAGEMENT_CALM_MAX) {
    targetTone = PersonaTone::Calm;
  } else if (engagement_ > HardwareConfig::Companion::PERSONA_PLAYFUL_ENGAGEMENT_MIN && affinity_ > HardwareConfig::Companion::PERSONA_PLAYFUL_AFFINITY_MIN && mood_ > HardwareConfig::Companion::PERSONA_PLAYFUL_MOOD_MIN) {
    targetTone = PersonaTone::Playful;
  }
  profile_.tone = targetTone;

  const float targetExpressiveness =
      0.28f + (engagement_ * HardwareConfig::Companion::PERSONA_LOW_ENGAGEMENT_CALM_MAX) + (mood_ * 0.10f) + (voicePreference_ * 0.12f) - (idleFactor * 0.12f);
  const float targetSociability =
      0.25f + (affinity_ * 0.40f) + (engagement_ * 0.20f) + (memorySignal_ * 0.08f);
  const float targetInitiative =
      0.22f + (engagement_ * 0.30f) + (mood_ * 0.15f) + (affinity_ * 0.10f) - (idleFactor * 0.10f);
  const float targetIntensity =
      0.30f + (engagement_ * 0.32f) + (voicePreference_ * 0.12f) - ((prefersCalm_ ? 1.0f : 0.0f) * 0.16f);
  const float targetProximity =
      0.24f + (affinity_ * 0.45f) + (memorySignal_ * 0.12f) + (engagement_ * 0.08f);

  profile_.expressiveness = approach(profile_.expressiveness, targetExpressiveness, HardwareConfig::Companion::PERSONA_ADAPT_RATE_EXPRESSIVE, dtS);
  profile_.sociability = approach(profile_.sociability, targetSociability, HardwareConfig::Companion::PERSONA_ADAPT_RATE_SOCIABILITY, dtS);
  profile_.initiative = approach(profile_.initiative, targetInitiative, HardwareConfig::Companion::PERSONA_ADAPT_RATE_INITIATIVE, dtS);
  profile_.intensity = approach(profile_.intensity, targetIntensity, HardwareConfig::Companion::PERSONA_ADAPT_RATE_INTENSITY, dtS);
  profile_.socialProximity = approach(profile_.socialProximity, targetProximity, HardwareConfig::Companion::PERSONA_ADAPT_RATE_PROXIMITY, dtS);

  if (prefersCalm_ || profile_.tone == PersonaTone::Calm) {
    profile_.responseStyle = PersonaResponseStyle::Gentle;
  } else if (profile_.tone == PersonaTone::Playful && profile_.intensity > HardwareConfig::Companion::PERSONA_VOICE_PREF_PLAYFUL_MIN) {
    profile_.responseStyle = PersonaResponseStyle::Energetic;
  } else if (profile_.sociability < 0.32f) {
    profile_.responseStyle = PersonaResponseStyle::Reserved;
  } else {
    profile_.responseStyle = PersonaResponseStyle::Balanced;
  }

  profile_.clamp();

  if (profile_.tone != lastPublished_.tone ||
      profile_.responseStyle != lastPublished_.responseStyle ||
      fabsf(profile_.expressiveness - lastPublished_.expressiveness) > HardwareConfig::Companion::PERSONA_PUBLISH_DELTA ||
      fabsf(profile_.sociability - lastPublished_.sociability) > HardwareConfig::Companion::PERSONA_PUBLISH_DELTA ||
      fabsf(profile_.initiative - lastPublished_.initiative) > HardwareConfig::Companion::PERSONA_PUBLISH_DELTA ||
      fabsf(profile_.intensity - lastPublished_.intensity) > HardwareConfig::Companion::PERSONA_PUBLISH_DELTA ||
      fabsf(profile_.socialProximity - lastPublished_.socialProximity) > HardwareConfig::Companion::PERSONA_PUBLISH_DELTA) {
    publish(nowMs);
  }
}

void PersonaService::onEvent(const Event& event) {
  const unsigned long nowMs = (event.timestamp > 0) ? event.timestamp : millis();

  switch (event.type) {
    case EventType::EVT_MOOD_CHANGED:
      mood_ = static_cast<float>(event.value) / 1000.0f;
      break;

    case EventType::EVT_ENGAGEMENT_CHANGED:
      engagement_ = static_cast<float>(event.value) / 1000.0f;
      break;

    case EventType::EVT_AFFINITY_CHANGED:
      affinity_ = static_cast<float>(event.value) / 1000.0f;
      break;

    case EventType::EVT_PREFERENCE_UPDATED:
      prefersCalm_ = (event.value == static_cast<int>(AttentionFocus::Voice));
      break;

    case EventType::EVT_MEMORY_UPDATED:
      memorySignal_ = MathUtils::clamp(static_cast<float>(event.value) / HardwareConfig::Companion::PERSONA_MEMORY_SIGNAL_NORM, 0.0f, 1.0f);
      break;

    case EventType::EVT_VOICE_START:
    case EventType::EVT_VOICE_ACTIVITY:
      lastInteractionMs_ = nowMs;
      voicePreference_ = MathUtils::clamp(voicePreference_ + HardwareConfig::Companion::PERSONA_VOICE_PREF_INC, 0.0f, 1.0f);
      break;

    case EventType::EVT_TOUCH:
      lastInteractionMs_ = nowMs;
      voicePreference_ = MathUtils::clamp(voicePreference_ - HardwareConfig::Companion::PERSONA_TOUCH_PREF_DEC, 0.0f, 1.0f);
      break;

    default:
      break;
  }
}

const PersonaProfile& PersonaService::getProfile() const {
  return profile_;
}

void PersonaService::publish(unsigned long nowMs) {
  Event event;
  event.type = EventType::EVT_PERSONA_UPDATED;
  event.source = EventSource::PersonaService;
  event.value = static_cast<int>(profile_.tone);
  event.timestamp = nowMs;
  eventBus_.publish(event);

  lastPublished_ = profile_;
}

