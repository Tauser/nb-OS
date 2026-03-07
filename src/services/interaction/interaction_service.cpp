#include "interaction_service.h"

InteractionService::InteractionService(EventBus& eventBus, Diagnostics& diagnostics)
    : eventBus_(eventBus), diagnostics_(diagnostics) {}

void InteractionService::init() {
  eventBus_.subscribe(EventType::EVT_TOUCH, this);
  eventBus_.subscribe(EventType::EVT_SHAKE, this);
  eventBus_.subscribe(EventType::EVT_TILT, this);
  eventBus_.subscribe(EventType::EVT_FALL, this);
}

void InteractionService::update() {
}

void InteractionService::onEvent(const Event& event) {
  switch (event.type) {
    case EventType::EVT_TOUCH:
      touchCount_++;
      diagnostics_.logInfo("[SENSORS] EVT_TOUCH");
      break;
    case EventType::EVT_SHAKE:
      shakeCount_++;
      diagnostics_.logInfo("[SENSORS] EVT_SHAKE");
      break;
    case EventType::EVT_TILT:
      tiltCount_++;
      diagnostics_.logInfo("[SENSORS] EVT_TILT");
      break;
    case EventType::EVT_FALL:
      fallCount_++;
      diagnostics_.logInfo("[SENSORS] EVT_FALL");
      break;
    default:
      break;
  }
}
