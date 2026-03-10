#include "social_timing_service.h"

#include "../../config/hardware_config.h"
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

SocialTimingService::SocialTimingService(EventBus& eventBus, const IPersonaProvider& personaProvider)
    : eventBus_(eventBus), personaProvider_(personaProvider) {}

void SocialTimingService::init() {
  eventBus_.subscribe(EventType::EVT_ENGAGEMENT_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_ATTENTION_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_MOOD_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_MEMORY_UPDATED, this);
  eventBus_.subscribe(EventType::EVT_TOUCH, this);
  eventBus_.subscribe(EventType::EVT_VOICE_ACTIVITY, this);
  eventBus_.subscribe(EventType::EVT_INTENT_DETECTED, this);

  const unsigned long nowMs = millis();
  lastUpdateMs_ = nowMs;
  lastInteractionMs_ = nowMs;
  publish(nowMs);
}

void SocialTimingService::update(unsigned long nowMs) {
  if (nowMs - lastUpdateMs_ < HardwareConfig::Companion::SOCIAL_TIMING_UPDATE_INTERVAL_MS) {
    return;
  }

  const float dtS = static_cast<float>(nowMs - lastUpdateMs_) / 1000.0f;
  lastUpdateMs_ = nowMs;

  const PersonaProfile& persona = personaProvider_.getProfile();
  const float idleFactor = MathUtils::clamp(
      static_cast<float>(nowMs - lastInteractionMs_) / static_cast<float>(HardwareConfig::Homeostasis::LONG_IDLE_MS),
      0.0f,
      1.0f);

  float focusWeight = 0.45f;
  if (focus_ == AttentionFocus::Voice || focus_ == AttentionFocus::Touch) {
    focusWeight = 1.0f;
  } else if (focus_ == AttentionFocus::Vision) {
    focusWeight = 0.70f;
  } else if (focus_ == AttentionFocus::Internal) {
    focusWeight = 0.55f;
  }

  const float targetResponsiveness =
      (engagement_ * 0.45f) + (persona.initiative * 0.25f) + (focusWeight * 0.20f) + (memorySignal_ * 0.10f) - (idleFactor * 0.18f);
  const float targetInitiative =
      (engagement_ * 0.38f) + (persona.initiative * 0.32f) + (persona.sociability * 0.20f) + (mood_ * 0.10f) - (idleFactor * 0.22f);
  const float targetPersistence =
      (persona.socialProximity * 0.36f) + (memorySignal_ * 0.30f) + (engagement_ * 0.22f) - (idleFactor * 0.25f);
  const float targetPause =
      0.25f + (idleFactor * 0.45f) + ((1.0f - engagement_) * 0.25f) + ((focus_ == AttentionFocus::Idle) ? 0.10f : 0.0f);

  state_.responsiveness = approach(state_.responsiveness, targetResponsiveness, 0.30f, dtS);
  state_.initiative = approach(state_.initiative, targetInitiative, 0.25f, dtS);
  state_.persistence = approach(state_.persistence, targetPersistence, 0.20f, dtS);
  state_.pauseFactor = approach(state_.pauseFactor, targetPause, 0.22f, dtS);
  state_.clamp();

  if (fabsf(state_.responsiveness - lastPublished_.responsiveness) >= HardwareConfig::Companion::SOCIAL_TIMING_PUBLISH_THRESHOLD ||
      fabsf(state_.initiative - lastPublished_.initiative) >= HardwareConfig::Companion::SOCIAL_TIMING_PUBLISH_THRESHOLD ||
      fabsf(state_.persistence - lastPublished_.persistence) >= HardwareConfig::Companion::SOCIAL_TIMING_PUBLISH_THRESHOLD ||
      fabsf(state_.pauseFactor - lastPublished_.pauseFactor) >= HardwareConfig::Companion::SOCIAL_TIMING_PUBLISH_THRESHOLD) {
    publish(nowMs);
  }
}

void SocialTimingService::onEvent(const Event& event) {
  const unsigned long nowMs = (event.timestamp > 0) ? event.timestamp : millis();

  switch (event.type) {
    case EventType::EVT_ENGAGEMENT_CHANGED:
      engagement_ = MathUtils::clamp(static_cast<float>(event.value) / 1000.0f, 0.0f, 1.0f);
      break;

    case EventType::EVT_ATTENTION_CHANGED:
      focus_ = static_cast<AttentionFocus>(event.value);
      break;

    case EventType::EVT_MOOD_CHANGED: {
      const float valence = event.hasTypedPayload(EventPayloadKind::MoodChanged)
                                ? event.payload.moodChanged.valence
                                : (static_cast<float>(event.value) / 1000.0f);
      mood_ = MathUtils::clamp((valence + 1.0f) * 0.5f, 0.0f, 1.0f);
      break;
    }

    case EventType::EVT_MEMORY_UPDATED:
      memorySignal_ = MathUtils::clamp(static_cast<float>(event.value) / HardwareConfig::Companion::SOCIAL_TIMING_MEMORY_SIGNAL_NORM, 0.0f, 1.0f);
      break;

    case EventType::EVT_TOUCH:
    case EventType::EVT_VOICE_ACTIVITY:
    case EventType::EVT_INTENT_DETECTED:
      lastInteractionMs_ = nowMs;
      break;

    default:
      break;
  }
}

const SocialTimingState& SocialTimingService::getState() const {
  return state_;
}

void SocialTimingService::publish(unsigned long nowMs) {
  Event event;
  event.type = EventType::EVT_SOCIAL_TIMING_CHANGED;
  event.source = EventSource::SocialTimingService;
  event.value = static_cast<int>(state_.responsiveness * 1000.0f);
  event.timestamp = nowMs;
  eventBus_.publish(event);

  lastPublished_ = state_;
}

