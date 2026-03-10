#pragma once

#include "../core/event_types.h"
#include "event_payloads/emotion_changed_payload.h"
#include "event_payloads/health_status_payload.h"
#include "event_payloads/mood_changed_payload.h"
#include "event_payloads/power_status_payload.h"

#include <stdint.h>

enum class EventPayloadKind : uint8_t {
  None = 0,
  LegacyValue,
  EmotionChanged,
  MoodChanged,
  PowerStatus,
  HealthStatus
};

struct EventPayload {
  EmotionChangedPayload emotionChanged{};
  MoodChangedPayload moodChanged{};
  PowerStatusPayload powerStatus{};
  HealthStatusPayload healthStatus{};
};

struct Event {
  EventType type = EventType::None;
  EventSource source = EventSource::Unknown;
  int value = 0;
  uint32_t timestamp = 0;

  EventPayloadKind kind = EventPayloadKind::LegacyValue;
  EventPayload payload{};

  void setLegacyValue(int legacyValue) {
    value = legacyValue;
    kind = EventPayloadKind::LegacyValue;
  }

  void setEmotionChanged(const EmotionChangedPayload& typed) {
    payload.emotionChanged = typed;
    kind = EventPayloadKind::EmotionChanged;
    value = static_cast<int>(typed.arousal * 1000.0f);
  }

  void setMoodChanged(const MoodChangedPayload& typed) {
    payload.moodChanged = typed;
    kind = EventPayloadKind::MoodChanged;
    value = static_cast<int>(typed.valence * 1000.0f);
  }

  void setPowerStatus(const PowerStatusPayload& typed) {
    payload.powerStatus = typed;
    kind = EventPayloadKind::PowerStatus;
    value = typed.batteryPercent;
  }

  void setHealthStatus(const HealthStatusPayload& typed) {
    payload.healthStatus = typed;
    kind = EventPayloadKind::HealthStatus;
    value = typed.healthScore;
  }

  bool hasTypedPayload(EventPayloadKind expectedKind) const {
    return kind == expectedKind;
  }
};
