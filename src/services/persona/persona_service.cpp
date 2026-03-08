#include "persona_service.h"

#include "../../config/hardware_config.h"
#include "../../models/attention_focus.h"
#include "../../models/event.h"
#include <Arduino.h>
#include <math.h>

PersonaService::PersonaService(EventBus& eventBus)
    : eventBus_(eventBus) {}

void PersonaService::init() {
  eventBus_.subscribe(EventType::EVT_MOOD_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_ENGAGEMENT_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_AFFINITY_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_PREFERENCE_UPDATED, this);

  lastUpdateMs_ = millis();
  publish(lastUpdateMs_);
}

void PersonaService::update(unsigned long nowMs) {
  if (nowMs - lastUpdateMs_ < HardwareConfig::Companion::PERSONA_UPDATE_INTERVAL_MS) {
    return;
  }

  lastUpdateMs_ = nowMs;

  if (prefersCalm_ || mood_ < -0.20f) {
    profile_.tone = PersonaTone::Calm;
  } else if (engagement_ > 0.60f && affinity_ > 0.45f && mood_ > 0.10f) {
    profile_.tone = PersonaTone::Playful;
  } else {
    profile_.tone = PersonaTone::Warm;
  }

  profile_.expressiveness = 0.35f + (engagement_ * 0.45f) + (mood_ * 0.15f);
  if (profile_.expressiveness < 0.0f) profile_.expressiveness = 0.0f;
  if (profile_.expressiveness > 1.0f) profile_.expressiveness = 1.0f;

  profile_.sociability = 0.30f + (affinity_ * 0.50f) + (engagement_ * 0.20f);
  if (profile_.sociability < 0.0f) profile_.sociability = 0.0f;
  if (profile_.sociability > 1.0f) profile_.sociability = 1.0f;

  if (profile_.tone != lastPublished_.tone ||
      fabsf(profile_.expressiveness - lastPublished_.expressiveness) > 0.08f ||
      fabsf(profile_.sociability - lastPublished_.sociability) > 0.08f) {
    publish(nowMs);
  }
}

void PersonaService::onEvent(const Event& event) {
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
