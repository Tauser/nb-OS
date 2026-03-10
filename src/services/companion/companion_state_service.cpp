#include "companion_state_service.h"

#include "../../config/hardware_config.h"
#include "../../models/event.h"
#include "../../utils/math_utils.h"
#include <Arduino.h>
#include <math.h>

CompanionStateService::CompanionStateService(EventBus& eventBus, const ISocialTimingProvider& socialTimingProvider)
    : eventBus_(eventBus), socialTimingProvider_(socialTimingProvider) {}

void CompanionStateService::init() {
  eventBus_.subscribe(EventType::EVT_MOOD_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_MOOD_PROFILE_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_ENGAGEMENT_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_AFFINITY_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_ATTENTION_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_ROUTINE_STATE_CHANGED, this);

  const unsigned long nowMs = millis();
  state_.lastUpdateMs = nowMs;
  state_.refreshDerived();
  state_.valid = true;

  lastPublished_ = state_;
  publish(nowMs);
}

void CompanionStateService::update(unsigned long nowMs) {
  const SocialTimingState& timing = socialTimingProvider_.getState();
  state_.socialResponsiveness = timing.responsiveness;
  state_.socialInitiative = timing.initiative;
  state_.socialPersistence = timing.persistence;
  state_.socialPauseFactor = timing.pauseFactor;

  state_.refreshDerived();
  state_.lastUpdateMs = nowMs;
  state_.valid = true;

  const float deltaReadiness = fabsf(state_.socialReadiness - lastPublished_.socialReadiness);
  const float deltaInitiative = fabsf(state_.initiativeReadiness - lastPublished_.initiativeReadiness);
  if (deltaReadiness >= HardwareConfig::Companion::SOCIAL_TIMING_PUBLISH_THRESHOLD ||
      deltaInitiative >= HardwareConfig::Companion::SOCIAL_TIMING_PUBLISH_THRESHOLD) {
    publish(nowMs);
  }
}

void CompanionStateService::onEvent(const Event& event) {
  switch (event.type) {
    case EventType::EVT_MOOD_CHANGED:
      if (event.hasTypedPayload(EventPayloadKind::MoodChanged)) {
        state_.moodValence = event.payload.moodChanged.valence;
      } else {
        state_.moodValence = MathUtils::clamp(static_cast<float>(event.value) / 1000.0f, -1.0f, 1.0f);
      }
      break;

    case EventType::EVT_MOOD_PROFILE_CHANGED:
      state_.moodProfile = static_cast<MoodProfile>(event.value);
      break;

    case EventType::EVT_ENGAGEMENT_CHANGED:
      state_.engagement = MathUtils::clamp(static_cast<float>(event.value) / 1000.0f, 0.0f, 1.0f);
      break;

    case EventType::EVT_AFFINITY_CHANGED:
      state_.affinityBond = MathUtils::clamp(static_cast<float>(event.value) / 1000.0f, 0.0f, 1.0f);
      break;

    case EventType::EVT_ATTENTION_CHANGED:
      state_.attentionFocus = static_cast<AttentionFocus>(event.value);
      break;

    case EventType::EVT_ROUTINE_STATE_CHANGED:
      state_.routineState = static_cast<RoutineState>(event.value);
      break;

    default:
      break;
  }
}

const CompanionState& CompanionStateService::getState() const {
  return state_;
}

void CompanionStateService::publish(unsigned long nowMs) {
  Event event;
  event.type = EventType::EVT_COMPANION_STATE_CHANGED;
  event.source = EventSource::CompanionStateService;
  event.value = static_cast<int>(state_.socialReadiness * 1000.0f);
  event.timestamp = nowMs;
  eventBus_.publish(event);

  lastPublished_ = state_;
}
