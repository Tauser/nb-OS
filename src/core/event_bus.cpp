#include "event_bus.h"

EventBus::EventBus() = default;

bool EventBus::subscribe(EventType type, IEventListener* listener) {
  if (listener == nullptr) {
    return false;
  }

  for (int i = 0; i < kMaxSubscribers; ++i) {
    if (!subscribers_[i].used) {
      subscribers_[i].used = true;
      subscribers_[i].type = type;
      subscribers_[i].listener = listener;
      return true;
    }
  }

  return false;
}

void EventBus::publish(const Event& event) {
  for (int i = 0; i < kMaxSubscribers; ++i) {
    if (subscribers_[i].used &&
        subscribers_[i].listener != nullptr &&
        subscribers_[i].type == event.type) {
      subscribers_[i].listener->onEvent(event);
    }
  }
}
