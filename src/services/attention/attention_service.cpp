#include "attention_service.h"

#include "../../config/hardware_config.h"
#include "../../models/event.h"
#include <Arduino.h>

AttentionService::AttentionService(EventBus& eventBus)
    : eventBus_(eventBus) {}

void AttentionService::init() {
  eventBus_.subscribe(EventType::Any, this);
  eventBus_.subscribe(EventType::EVT_ENGAGEMENT_CHANGED, this);

  const unsigned long nowMs = millis();
  focus_ = AttentionFocus::Idle;
  focusUntilMs_ = nowMs + HardwareConfig::Polish::ATTENTION_IDLE_RETURN_MS;
  lastFocusChangeMs_ = nowMs;
  lastIdleEnterMs_ = nowMs;
  lastInteractionMs_ = nowMs;
  lastInternalPulseMs_ = nowMs;
  lastScanAttemptMs_ = nowMs;
  publishFocus(focus_, nowMs);
}

void AttentionService::update(unsigned long nowMs) {
  const unsigned long dynamicScanMs =
      HardwareConfig::Polish::ATTENTION_SCAN_INTERVAL_MS +
      static_cast<unsigned long>((1.0f - engagement_) *
                                 static_cast<float>(HardwareConfig::Polish::ATTENTION_SCAN_IDLE_EXTRA_MS));

  if (focus_ != AttentionFocus::Idle && nowMs >= focusUntilMs_) {
    setFocus(AttentionFocus::Idle, nowMs, HardwareConfig::Polish::ATTENTION_IDLE_RETURN_MS);
    return;
  }

  const unsigned long silenceMs = nowMs - lastInteractionMs_;
  if (silenceMs < HardwareConfig::Polish::ATTENTION_IDLE_RETURN_MS) {
    return;
  }

  const bool idleDwellSatisfied =
      (focus_ == AttentionFocus::Idle) &&
      (nowMs - lastIdleEnterMs_ >= HardwareConfig::Polish::ATTENTION_MIN_IDLE_DWELL_MS);

  if (idleDwellSatisfied &&
      nowMs - lastScanAttemptMs_ >= dynamicScanMs &&
      nowMs - lastFocusChangeMs_ >= HardwareConfig::Polish::ATTENTION_MIN_FOCUS_SWITCH_MS) {
    lastScanAttemptMs_ = nowMs;
    lastInternalPulseMs_ = nowMs;
    setFocus(AttentionFocus::Internal,
             nowMs,
             HardwareConfig::Polish::ATTENTION_INTERNAL_HOLD_MS);
  }
}

void AttentionService::onEvent(const Event& event) {
  if (event.source == EventSource::AttentionService ||
      event.type == EventType::EVT_ATTENTION_CHANGED ||
      event.type == EventType::None) {
    return;
  }

  const unsigned long nowMs = (event.timestamp > 0) ? event.timestamp : millis();
  const float engagementScale = 0.85f + (engagement_ * 0.45f);
  const unsigned long dynamicHoldMs = static_cast<unsigned long>(
      static_cast<float>(HardwareConfig::Polish::ATTENTION_HOLD_MS) * engagementScale);

  switch (event.type) {
    case EventType::EVT_TOUCH:
      lastInteractionMs_ = nowMs;
      setFocus(AttentionFocus::Touch, nowMs, dynamicHoldMs);
      break;

    case EventType::EVT_VOICE_START:
    case EventType::EVT_VOICE_ACTIVITY:
    case EventType::EVT_INTENT_DETECTED:
      lastInteractionMs_ = nowMs;
      setFocus(AttentionFocus::Voice, nowMs, dynamicHoldMs);
      break;

    case EventType::EVT_VISION_MOTION:
    case EventType::EVT_VISION_DARK:
      setFocus(AttentionFocus::Vision, nowMs, dynamicHoldMs);
      break;

    case EventType::EVT_POWER_MODE_CHANGED:
      setFocus(AttentionFocus::Power, nowMs, dynamicHoldMs);
      break;

    case EventType::EVT_ENGAGEMENT_CHANGED:
      engagement_ = static_cast<float>(event.value) / 1000.0f;
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

  const bool highPriorityFocus =
      (focus == AttentionFocus::Touch ||
       focus == AttentionFocus::Voice ||
       focus == AttentionFocus::Power);

  if (!highPriorityFocus &&
      focus_ != focus &&
      nowMs - lastFocusChangeMs_ < HardwareConfig::Polish::ATTENTION_MIN_FOCUS_SWITCH_MS) {
    return;
  }

  focus_ = focus;
  focusUntilMs_ = nowMs + holdMs;
  lastFocusChangeMs_ = nowMs;
  if (focus_ == AttentionFocus::Idle) {
    lastIdleEnterMs_ = nowMs;
  }

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
