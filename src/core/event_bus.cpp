#include "event_bus.h"

#include "../config/feature_flags.h"

#include <Arduino.h>

#ifndef NCOS_SIM_MODE
#define NCOS_SIM_MODE 0
#endif

namespace {
constexpr unsigned long kTraceWindowMs = 1000UL;
}

EventBus::EventBus() = default;

bool EventBus::subscribe(EventType type, IEventListener* listener) {
  if (listener == nullptr || type == EventType::None) {
    return false;
  }

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

  const bool traceEnabled = FeatureFlags::EVENTBUS_V2_ENABLED;
  const unsigned long nowMs = millis();

  if (traceEnabled && traceWindowStartMs_ == 0UL) {
    resetTraceWindow(nowMs);
  }

  const int traceIdx = typeToTraceIndex(event.type);
  if (traceEnabled && traceIdx >= 0) {
    trace_[traceIdx].published++;
  }

  for (int i = 0; i < kMaxSubscribers; ++i) {
    if (!subscribers_[i].used || subscribers_[i].listener == nullptr) {
      continue;
    }

    if (subscribers_[i].type == EventType::Any || subscribers_[i].type == event.type) {
      if (traceEnabled && traceIdx >= 0) {
        const unsigned long t0 = micros();
        subscribers_[i].listener->onEvent(event);
        const uint32_t elapsedUs = static_cast<uint32_t>(micros() - t0);

        trace_[traceIdx].handled++;
        trace_[traceIdx].handlerTotalUs += elapsedUs;
        if (elapsedUs > trace_[traceIdx].handlerMaxUs) {
          trace_[traceIdx].handlerMaxUs = elapsedUs;
        }
      } else {
        subscribers_[i].listener->onEvent(event);
      }
    }
  }

  if (traceEnabled && (nowMs - traceWindowStartMs_ >= kTraceWindowMs)) {
    flushTraceWindow(nowMs);
  }
}

void EventBus::clear() {
  for (int i = 0; i < kMaxSubscribers; ++i) {
    subscribers_[i] = SubscriberSlot{};
  }
}

int EventBus::typeToTraceIndex(EventType type) const {
  const int idx = static_cast<int>(type);
  if (idx < 0 || idx >= kTraceTypeSlots) {
    return -1;
  }
  return idx;
}

void EventBus::resetTraceWindow(unsigned long nowMs) {
  traceWindowStartMs_ = nowMs;
  for (int i = 0; i < kTraceTypeSlots; ++i) {
    trace_[i] = TraceSlot{};
  }
}

void EventBus::flushTraceWindow(unsigned long nowMs) {
#if NCOS_SIM_MODE
  int topIdx[3] = {-1, -1, -1};

  for (int i = 0; i < kTraceTypeSlots; ++i) {
    if (trace_[i].published == 0) {
      continue;
    }

    if (topIdx[0] < 0 || trace_[i].published > trace_[topIdx[0]].published) {
      topIdx[2] = topIdx[1];
      topIdx[1] = topIdx[0];
      topIdx[0] = i;
    } else if (topIdx[1] < 0 || trace_[i].published > trace_[topIdx[1]].published) {
      topIdx[2] = topIdx[1];
      topIdx[1] = i;
    } else if (topIdx[2] < 0 || trace_[i].published > trace_[topIdx[2]].published) {
      topIdx[2] = i;
    }
  }

  for (int rank = 0; rank < 3; ++rank) {
    const int idx = topIdx[rank];
    if (idx < 0) {
      continue;
    }

    const TraceSlot& slot = trace_[idx];
    const uint32_t avgUs = (slot.handled > 0) ? (slot.handlerTotalUs / slot.handled) : 0;
    Serial.printf("[EVT_TRACE] rank=%d type=%d eps=%lu handlers=%lu avg_us=%lu max_us=%lu\n",
                  rank + 1,
                  idx,
                  static_cast<unsigned long>(slot.published),
                  static_cast<unsigned long>(slot.handled),
                  static_cast<unsigned long>(avgUs),
                  static_cast<unsigned long>(slot.handlerMaxUs));
  }
#else
  (void)nowMs;
#endif

  resetTraceWindow(nowMs);
}
