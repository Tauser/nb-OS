#include "mood_service.h"

#include "../../config/hardware_config.h"
#include "../../models/event.h"
#include <Arduino.h>
#include <math.h>

MoodService::MoodService(EventBus& eventBus, const IEmotionProvider& emotionProvider)
    : eventBus_(eventBus), emotionProvider_(emotionProvider) {}

void MoodService::init() {
  eventBus_.subscribe(EventType::EVT_TOUCH, this);
  eventBus_.subscribe(EventType::EVT_VOICE_ACTIVITY, this);
  eventBus_.subscribe(EventType::EVT_FALL, this);

  const unsigned long nowMs = millis();
  lastUpdateMs_ = nowMs;
  publishMood(nowMs);
}

void MoodService::update(unsigned long nowMs) {
  if (nowMs - lastUpdateMs_ < HardwareConfig::Companion::MOOD_UPDATE_INTERVAL_MS) {
    return;
  }

  const float dtS = static_cast<float>(nowMs - lastUpdateMs_) / 1000.0f;
  lastUpdateMs_ = nowMs;

  const EmotionState& emo = emotionProvider_.getEmotionState();
  state_.valence += (emo.valence - state_.valence) * 0.08f;
  state_.valence += (0.0f - state_.valence) * (HardwareConfig::Companion::MOOD_DECAY_PER_S * dtS);
  state_.stability = 1.0f - fabsf(emo.arousal - 0.5f);
  state_.clamp();

  if (fabsf(state_.valence - lastPublished_.valence) >= HardwareConfig::Companion::MOOD_PUBLISH_THRESHOLD) {
    publishMood(nowMs);
  }
}

void MoodService::onEvent(const Event& event) {
  switch (event.type) {
    case EventType::EVT_TOUCH:
      state_.valence += 0.05f;
      break;
    case EventType::EVT_VOICE_ACTIVITY:
      state_.valence += 0.03f;
      break;
    case EventType::EVT_FALL:
      state_.valence -= 0.10f;
      break;
    default:
      break;
  }

  state_.clamp();
}

const MoodState& MoodService::getState() const {
  return state_;
}

void MoodService::publishMood(unsigned long nowMs) {
  Event event;
  event.type = EventType::EVT_MOOD_CHANGED;
  event.source = EventSource::MoodService;
  event.value = static_cast<int>(state_.valence * 1000.0f);
  event.timestamp = nowMs;
  eventBus_.publish(event);

  lastPublished_ = state_;
}
