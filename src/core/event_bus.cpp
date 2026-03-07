#include "event_bus.h"

EventBus::EventBus() = default;

bool EventBus::subscribe(EventType type, IEventListener* listener) {
  if (listener == nullptr || type == EventType::None) {
    return false;
  }

  // Prevent duplicated subscription for the same listener and event type.
  for (int i = 0; i < kMaxSubscribers; ++i) {
    if (subscribers_[i].used && subscribers_[i].type == type && subscribers_[i].listener == listener) {
      return true;
    }
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

bool EventBus::unsubscribe(EventType type, IEventListener* listener) {
  if (listener == nullptr || type == EventType::None) {
    return false;
  }

  for (int i = 0; i < kMaxSubscribers; ++i) {
    if (subscribers_[i].used && subscribers_[i].type == type && subscribers_[i].listener == listener) {
      subscribers_[i] = SubscriberSlot{};
      return true;
    }
  }

  return false;
}

bool EventBus::unsubscribeAll(IEventListener* listener) {
  if (listener == nullptr) {
    return false;
  }

  bool removed = false;

  for (int i = 0; i < kMaxSubscribers; ++i) {
    if (subscribers_[i].used && subscribers_[i].listener == listener) {
      subscribers_[i] = SubscriberSlot{};
      removed = true;
    }
  }

  return removed;
}

void EventBus::publish(const Event& event) {
  if (event.type == EventType::None) {
    return;
  }

  for (int i = 0; i < kMaxSubscribers; ++i) {
    if (!subscribers_[i].used || subscribers_[i].listener == nullptr) {
      continue;
    }

    if (subscribers_[i].type == EventType::Any || subscribers_[i].type == event.type) {
      subscribers_[i].listener->onEvent(event);
    }
  }
}

void EventBus::clear() {
  for (int i = 0; i < kMaxSubscribers; ++i) {
    subscribers_[i] = SubscriberSlot{};
  }
}
