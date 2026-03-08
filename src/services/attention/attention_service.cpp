#include "attention_service.h"

#include "../../config/hardware_config.h"
#include "../../models/event.h"
#include <Arduino.h>

AttentionService::AttentionService(EventBus& eventBus)
    : eventBus_(eventBus) {}

void AttentionService::init() {
  eventBus_.subscribe(EventType::Any, this);

  const unsigned long nowMs = millis();
  focus_ = AttentionFocus::Idle;
  focusUntilMs_ = nowMs + HardwareConfig::Polish::ATTENTION_IDLE_RETURN_MS;
  lastInteractionMs_ = nowMs;
  lastInternalPulseMs_ = nowMs;
  lastScanAttemptMs_ = nowMs;
  publishFocus(focus_, nowMs);
}

void AttentionService::update(unsigned long nowMs) {
  if (focus_ != AttentionFocus::Idle && nowMs >= focusUntilMs_) {
    setFocus(AttentionFocus::Idle, nowMs, HardwareConfig::Polish::ATTENTION_IDLE_RETURN_MS);
    return;
  }

  const unsigned long silenceMs = nowMs - lastInteractionMs_;
  if (silenceMs < HardwareConfig::Polish::ATTENTION_IDLE_RETURN_MS) {
    return;
  }

  if (nowMs - lastScanAttemptMs_ >= HardwareConfig::Polish::ATTENTION_SCAN_INTERVAL_MS) {
    lastScanAttemptMs_ = nowMs;
    lastInternalPulseMs_ = nowMs;
    setFocus(AttentionFocus::Internal, nowMs, HardwareConfig::Polish::ATTENTION_INTERNAL_PULSE_MS);
    return;
  }

  if (focus_ == AttentionFocus::Idle &&
      nowMs - lastInternalPulseMs_ >= HardwareConfig::Polish::ATTENTION_INTERNAL_PULSE_MS) {
    lastInternalPulseMs_ = nowMs;
    setFocus(AttentionFocus::Internal, nowMs, HardwareConfig::Polish::ATTENTION_HOLD_MS);
  }
}

void AttentionService::onEvent(const Event& event) {
  if (event.source == EventSource::AttentionService ||
      event.type == EventType::EVT_ATTENTION_CHANGED ||
      event.type == EventType::None) {
    return;
  }

  const unsigned long nowMs = (event.timestamp > 0) ? event.timestamp : millis();

  switch (event.type) {
    case EventType::EVT_TOUCH:
      lastInteractionMs_ = nowMs;
      setFocus(AttentionFocus::Touch, nowMs, HardwareConfig::Polish::ATTENTION_HOLD_MS);
      break;

    case EventType::EVT_VOICE_START:
    case EventType::EVT_VOICE_ACTIVITY:
    case EventType::EVT_INTENT_DETECTED:
      lastInteractionMs_ = nowMs;
      setFocus(AttentionFocus::Voice, nowMs, HardwareConfig::Polish::ATTENTION_HOLD_MS);
      break;

    case EventType::EVT_VISION_MOTION:
    case EventType::EVT_VISION_DARK:
      setFocus(AttentionFocus::Vision, nowMs, HardwareConfig::Polish::ATTENTION_HOLD_MS);
      break;

    case EventType::EVT_POWER_MODE_CHANGED:
      setFocus(AttentionFocus::Power, nowMs, HardwareConfig::Polish::ATTENTION_HOLD_MS);
      break;

    case EventType::EVT_BEHAVIOR_ACTION:
      setFocus(AttentionFocus::Internal, nowMs, HardwareConfig::Polish::ATTENTION_HOLD_MS);
      break;

    case EventType::EVT_IDLE:
      if (focus_ == AttentionFocus::Idle) {
        focusUntilMs_ = nowMs + HardwareConfig::Polish::ATTENTION_IDLE_RETURN_MS;
      }
      break;

    default:
      break;
  }
}

AttentionFocus AttentionService::getFocus() const {
  return focus_;
}

void AttentionService::setFocus(AttentionFocus focus, unsigned long nowMs, unsigned long holdMs) {
  if (focus_ == focus && nowMs < focusUntilMs_) {
    focusUntilMs_ = nowMs + holdMs;
    return;
  }

  focus_ = focus;
  focusUntilMs_ = nowMs + holdMs;
  publishFocus(focus, nowMs);
}

void AttentionService::publishFocus(AttentionFocus focus, unsigned long nowMs) {
  Event event;
  event.type = EventType::EVT_ATTENTION_CHANGED;
  event.source = EventSource::AttentionService;
  event.value = static_cast<int>(focus);
  event.timestamp = nowMs;
  eventBus_.publish(event);
}
