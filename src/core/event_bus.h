#pragma once

#include "../interfaces/i_event_listener.h"
#include "../models/event.h"

class EventBus {
public:
  static constexpr int kMaxSubscribers = 96;
  static constexpr int kTraceTypeSlots = 96;

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

  struct TraceSlot {
    uint32_t published = 0;
    uint32_t handled = 0;
    uint32_t handlerTotalUs = 0;
    uint32_t handlerMaxUs = 0;
  };

  int typeToTraceIndex(EventType type) const;
  void resetTraceWindow(unsigned long nowMs);
  void flushTraceWindow(unsigned long nowMs);

  SubscriberSlot subscribers_[kMaxSubscribers];
  TraceSlot trace_[kTraceTypeSlots];
  unsigned long traceWindowStartMs_ = 0;
};
