#include "preference_memory_service.h"

#include "../../config/hardware_config.h"
#include "../../models/event.h"
#include <Arduino.h>

PreferenceMemoryService::PreferenceMemoryService(EventBus& eventBus)
    : eventBus_(eventBus) {}

void PreferenceMemoryService::init() {
  eventBus_.subscribe(EventType::EVT_ATTENTION_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_INTENT_DETECTED, this);
  eventBus_.subscribe(EventType::EVT_ENGAGEMENT_CHANGED, this);

  lastUpdateMs_ = millis();
  publish(lastUpdateMs_);
}

void PreferenceMemoryService::update(unsigned long nowMs) {
  if (nowMs - lastUpdateMs_ < HardwareConfig::Companion::PREFERENCE_UPDATE_INTERVAL_MS) {
    return;
  }

  lastUpdateMs_ = nowMs;
  recalcPreferences();

  if (memory_.preferredFocus != lastPublished_.preferredFocus ||
      memory_.prefersCalmStyle != lastPublished_.prefersCalmStyle) {
    publish(nowMs);
  }
}

void PreferenceMemoryService::onEvent(const Event& event) {
  switch (event.type) {
    case EventType::EVT_ATTENTION_CHANGED: {
      const AttentionFocus focus = static_cast<AttentionFocus>(event.value);
      if (focus == AttentionFocus::Touch) {
        memory_.touchInteractions++;
      } else if (focus == AttentionFocus::Voice) {
        memory_.voiceInteractions++;
      } else if (focus == AttentionFocus::Vision) {
        memory_.visionInteractions++;
      }
      break;
    }

    case EventType::EVT_INTENT_DETECTED:
      memory_.voiceInteractions++;
      break;

    case EventType::EVT_ENGAGEMENT_CHANGED:
      memory_.prefersCalmStyle = (event.value < 350);
      break;

    default:
      break;
  }
}

const PreferenceMemory& PreferenceMemoryService::getMemory() const {
  return memory_;
}

void PreferenceMemoryService::recalcPreferences() {
  if (memory_.touchInteractions >= memory_.voiceInteractions &&
      memory_.touchInteractions >= memory_.visionInteractions) {
    memory_.preferredFocus = AttentionFocus::Touch;
  } else if (memory_.voiceInteractions >= memory_.visionInteractions) {
    memory_.preferredFocus = AttentionFocus::Voice;
  } else {
    memory_.preferredFocus = AttentionFocus::Vision;
  }
}

void PreferenceMemoryService::publish(unsigned long nowMs) {
  Event event;
  event.type = EventType::EVT_PREFERENCE_UPDATED;
  event.source = EventSource::PreferenceMemoryService;
  event.value = static_cast<int>(memory_.preferredFocus);
  event.timestamp = nowMs;
  eventBus_.publish(event);

  lastPublished_ = memory_;
}
