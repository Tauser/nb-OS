#include "engagement_service.h"

#include "../../config/hardware_config.h"
#include "../../models/event.h"
#include <Arduino.h>
#include <math.h>

EngagementService::EngagementService(EventBus& eventBus)
    : eventBus_(eventBus) {}

void EngagementService::init() {
  eventBus_.subscribe(EventType::EVT_TOUCH, this);
  eventBus_.subscribe(EventType::EVT_VOICE_ACTIVITY, this);
  eventBus_.subscribe(EventType::EVT_INTENT_DETECTED, this);

  lastUpdateMs_ = millis();
  publish(lastUpdateMs_);
}

void EngagementService::update(unsigned long nowMs) {
  if (nowMs - lastUpdateMs_ < HardwareConfig::Companion::ENGAGEMENT_UPDATE_INTERVAL_MS) {
    return;
  }

  const float dtS = static_cast<float>(nowMs - lastUpdateMs_) / 1000.0f;
  lastUpdateMs_ = nowMs;

  state_.score -= HardwareConfig::Companion::ENGAGEMENT_DECAY_PER_S * dtS;
  state_.clamp();

  if (fabsf(state_.score - lastPublished_) >= HardwareConfig::Companion::ENGAGEMENT_PUBLISH_THRESHOLD) {
    publish(nowMs);
  }
}

void EngagementService::onEvent(const Event& event) {
  switch (event.type) {
    case EventType::EVT_TOUCH:
      state_.score += 0.12f;
      break;
    case EventType::EVT_VOICE_ACTIVITY:
      state_.score += 0.08f;
      break;
    case EventType::EVT_INTENT_DETECTED:
      state_.score += 0.06f;
      break;
    default:
      break;
  }

  state_.clamp();
}

const EngagementState& EngagementService::getState() const {
  return state_;
}

void EngagementService::publish(unsigned long nowMs) {
  Event event;
  event.type = EventType::EVT_ENGAGEMENT_CHANGED;
  event.source = EventSource::EngagementService;
  event.value = static_cast<int>(state_.score * 1000.0f);
  event.timestamp = nowMs;
  eventBus_.publish(event);

  lastPublished_ = state_.score;
}
