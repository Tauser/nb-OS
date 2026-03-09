#include "engagement_service.h"

#include "../../config/hardware_config.h"
#include "../../models/event.h"
#include "../../utils/math_utils.h"
#include <Arduino.h>
#include <math.h>

EngagementService::EngagementService(EventBus& eventBus)
    : eventBus_(eventBus) {}

void EngagementService::init() {
  eventBus_.subscribe(EventType::EVT_TOUCH, this);
  eventBus_.subscribe(EventType::EVT_VOICE_ACTIVITY, this);
  eventBus_.subscribe(EventType::EVT_INTENT_DETECTED, this);
  eventBus_.subscribe(EventType::EVT_ATTENTION_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_MEMORY_UPDATED, this);

  const unsigned long nowMs = millis();
  lastUpdateMs_ = nowMs;
  lastInteractionMs_ = nowMs;
  publish(nowMs);
}

void EngagementService::update(unsigned long nowMs) {
  if (nowMs - lastUpdateMs_ < HardwareConfig::Companion::ENGAGEMENT_UPDATE_INTERVAL_MS) {
    return;
  }

  const float dtS = static_cast<float>(nowMs - lastUpdateMs_) / 1000.0f;
  lastUpdateMs_ = nowMs;

  const float idleFactor = MathUtils::clamp(
      static_cast<float>(nowMs - lastInteractionMs_) / static_cast<float>(HardwareConfig::Homeostasis::LONG_IDLE_MS),
      0.0f,
      1.0f);

  float focusWeight = 0.55f;
  if (focus_ == AttentionFocus::Voice || focus_ == AttentionFocus::Touch) {
    focusWeight = 1.0f;
  } else if (focus_ == AttentionFocus::Vision) {
    focusWeight = 0.72f;
  } else if (focus_ == AttentionFocus::Internal) {
    focusWeight = 0.48f;
  }

  const float adaptiveDecay = HardwareConfig::Companion::ENGAGEMENT_DECAY_PER_S *
                              (1.0f + (idleFactor * 0.60f)) *
                              (1.0f - (memorySignal_ * 0.20f));

  state_.score -= adaptiveDecay * dtS;
  state_.score += ((focusWeight - 0.55f) * 0.02f * dtS);
  state_.score += (memorySignal_ * 0.01f * dtS);
  state_.clamp();

  if (fabsf(state_.score - lastPublished_) >= HardwareConfig::Companion::ENGAGEMENT_PUBLISH_THRESHOLD) {
    publish(nowMs);
  }
}

void EngagementService::onEvent(const Event& event) {
  const unsigned long nowMs = (event.timestamp > 0) ? event.timestamp : millis();

  switch (event.type) {
    case EventType::EVT_TOUCH:
      state_.score += 0.12f;
      lastInteractionMs_ = nowMs;
      break;

    case EventType::EVT_VOICE_ACTIVITY:
      state_.score += 0.08f;
      lastInteractionMs_ = nowMs;
      break;

    case EventType::EVT_INTENT_DETECTED:
      state_.score += 0.06f;
      lastInteractionMs_ = nowMs;
      break;

    case EventType::EVT_ATTENTION_CHANGED:
      focus_ = static_cast<AttentionFocus>(event.value);
      break;

    case EventType::EVT_MEMORY_UPDATED:
      memorySignal_ = MathUtils::clamp(static_cast<float>(event.value) / HardwareConfig::Companion::SOCIAL_TIMING_MEMORY_SIGNAL_NORM, 0.0f, 1.0f);
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
