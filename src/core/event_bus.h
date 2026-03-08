#pragma once

#include "../interfaces/i_event_listener.h"
#include "../models/event.h"

class EventBus {
public:
  static constexpr int kMaxSubscribers = 32;

  EventBus();

  bool subscribe(EventType type, IEventListener* listener);
  bool unsubscribe(EventType type, IEventListener* listener);
  bool unsubscribeAll(IEventListener* listener);
  void publish(const Event& event);
  void clear();

private:
  struct SubscriberSlot {
    bool used = false;
    EventType type = EventType::None;
    IEventListener* listener = nullptr;
  };

  SubscriberSlot subscribers_[kMaxSubscribers];
};

