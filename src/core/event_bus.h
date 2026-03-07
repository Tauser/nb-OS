#pragma once
#include "../models/event.h"
#include "../interfaces/i_event_listener.h"

class EventBus {
public:
  static constexpr int kMaxSubscribers = 16;

  EventBus();
  bool subscribe(EventType type, IEventListener* listener);
  void publish(const Event& event);

private:
  struct SubscriberSlot {
    bool used = false;
    EventType type = EventType::None;
    IEventListener* listener = nullptr;
  };

  SubscriberSlot subscribers_[kMaxSubscribers];
};
