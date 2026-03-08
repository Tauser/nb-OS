#include "routine_service.h"

#include "../../config/hardware_config.h"
#include "../../models/event.h"
#include "../../models/power_state.h"
#include <Arduino.h>

RoutineService::RoutineService(EventBus& eventBus, IFaceController& faceController, IMotion& motion)
    : eventBus_(eventBus), faceController_(faceController), motion_(motion) {}

void RoutineService::init() {
  eventBus_.subscribe(EventType::EVT_IDLE, this);
  eventBus_.subscribe(EventType::EVT_VOICE_START, this);
  eventBus_.subscribe(EventType::EVT_POWER_MODE_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_CHARGING_STATE_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_TOUCH, this);

  const unsigned long nowMs = millis();
  lastUpdateMs_ = nowMs;
  lastInteractionMs_ = nowMs;
  setState(RoutineState::Idle, nowMs);
}

void RoutineService::update(unsigned long nowMs) {
  if (nowMs - lastUpdateMs_ < HardwareConfig::Polish::ROUTINE_UPDATE_INTERVAL_MS) {
    return;
  }

  lastUpdateMs_ = nowMs;

  if (state_ == RoutineState::Idle &&
      (nowMs - lastInteractionMs_ > HardwareConfig::Polish::ROUTINE_IDLE_TRIGGER_MS)) {
    faceController_.requestExpression(ExpressionType::Neutral, EyeAnimPriority::Idle, 300);
    motion_.idleSway();
  }
}

void RoutineService::onEvent(const Event& event) {
  const unsigned long nowMs = (event.timestamp > 0) ? event.timestamp : millis();

  switch (event.type) {
    case EventType::EVT_TOUCH:
      lastInteractionMs_ = nowMs;
      setState(RoutineState::Resume, nowMs);
      faceController_.requestExpression(ExpressionType::FaceRecognized, EyeAnimPriority::Social, 450);
      motion_.idleSway();
      break;

    case EventType::EVT_VOICE_START:
      lastInteractionMs_ = nowMs;
      setState(RoutineState::Listening, nowMs);
      faceController_.requestExpression(ExpressionType::FaceRecognized, EyeAnimPriority::Social, 550);
      motion_.softListen();
      break;

    case EventType::EVT_POWER_MODE_CHANGED: {
      const PowerMode mode = static_cast<PowerMode>(event.value);
      if (mode == PowerMode::Sleep) {
        setState(RoutineState::Rest, nowMs);
        faceController_.requestExpression(ExpressionType::Sad, EyeAnimPriority::Emotion, 700);
        motion_.center();
      }
      break;
    }

    case EventType::EVT_CHARGING_STATE_CHANGED:
      if (event.value != 0) {
        setState(RoutineState::Charging, nowMs);
        faceController_.requestExpression(ExpressionType::FaceRecognized, EyeAnimPriority::Emotion, 600);
        motion_.center();
      } else {
        setState(RoutineState::Resume, nowMs);
      }
      break;

    case EventType::EVT_IDLE:
      if (state_ == RoutineState::Resume || state_ == RoutineState::Listening) {
        setState(RoutineState::Idle, nowMs);
      }
      break;

    default:
      break;
  }
}

void RoutineService::setState(RoutineState state, unsigned long nowMs) {
  if (state_ == state) {
    return;
  }

  state_ = state;
  publishRoutineState(nowMs);
}

void RoutineService::publishRoutineState(unsigned long nowMs) {
  Event event;
  event.type = EventType::EVT_ROUTINE_STATE_CHANGED;
  event.source = EventSource::RoutineService;
  event.value = static_cast<int>(state_);
  event.timestamp = nowMs;
  eventBus_.publish(event);
}
