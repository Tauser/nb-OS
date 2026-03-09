#include "affinity_service.h"

#include "../../config/hardware_config.h"
#include "../../models/attention_focus.h"
#include "../../models/event.h"
#include "../../utils/math_utils.h"
#include <Arduino.h>
#include <math.h>

AffinityService::AffinityService(EventBus& eventBus)
    : eventBus_(eventBus) {}

void AffinityService::init() {
  eventBus_.subscribe(EventType::EVT_TOUCH, this);
  eventBus_.subscribe(EventType::EVT_VOICE_ACTIVITY, this);
  eventBus_.subscribe(EventType::EVT_INTENT_DETECTED, this);
  eventBus_.subscribe(EventType::EVT_FALL, this);
  eventBus_.subscribe(EventType::EVT_MEMORY_UPDATED, this);
  eventBus_.subscribe(EventType::EVT_PREFERENCE_UPDATED, this);

  lastUpdateMs_ = millis();
  publish(lastUpdateMs_);
}

void AffinityService::update(unsigned long nowMs) {
  if (nowMs - lastUpdateMs_ < HardwareConfig::Companion::AFFINITY_UPDATE_INTERVAL_MS) {
    return;
  }

  const float dtS = static_cast<float>(nowMs - lastUpdateMs_) / 1000.0f;
  lastUpdateMs_ = nowMs;

  const float adaptiveDecay = HardwareConfig::Companion::AFFINITY_DECAY_PER_S *
                              (1.0f - (memorySignal_ * HardwareConfig::Companion::AFFINITY_MEMORY_DECAY_REDUCTION)) *
                              (prefersVoice_ ? HardwareConfig::Companion::AFFINITY_VOICE_DECAY_SCALE : 1.0f);

  state_.bond -= adaptiveDecay * dtS;
  state_.clamp();

  if (fabsf(state_.bond - lastPublished_) >= HardwareConfig::Companion::AFFINITY_PUBLISH_THRESHOLD) {
    publish(nowMs);
  }
}

void AffinityService::onEvent(const Event& event) {
  switch (event.type) {
    case EventType::EVT_TOUCH:
      state_.bond += prefersVoice_ ? 0.035f : 0.045f;
      break;

    case EventType::EVT_VOICE_ACTIVITY:
      state_.bond += prefersVoice_ ? 0.030f : 0.020f;
      break;

    case EventType::EVT_INTENT_DETECTED:
      state_.bond += 0.020f + (memorySignal_ * 0.008f);
      break;

    case EventType::EVT_FALL:
      state_.bond -= 0.030f;
      break;

    case EventType::EVT_MEMORY_UPDATED:
      memorySignal_ = MathUtils::clamp(static_cast<float>(event.value) / HardwareConfig::Companion::AFFINITY_MEMORY_SIGNAL_NORM, 0.0f, 1.0f);
      break;

    case EventType::EVT_PREFERENCE_UPDATED:
      prefersVoice_ = (static_cast<AttentionFocus>(event.value) == AttentionFocus::Voice);
      break;

    default:
      break;
  }

  state_.clamp();
}

const AffinityState& AffinityService::getState() const {
  return state_;
}

void AffinityService::publish(unsigned long nowMs) {
  Event event;
  event.type = EventType::EVT_AFFINITY_CHANGED;
  event.source = EventSource::AffinityService;
  event.value = static_cast<int>(state_.bond * 1000.0f);
  event.timestamp = nowMs;
  eventBus_.publish(event);

  lastPublished_ = state_.bond;
}

