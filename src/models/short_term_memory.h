#pragma once

#include "../core/event_types.h"
#include <stdint.h>

struct ShortTermMemory {
  static constexpr uint8_t kMaxRecentEvents = 8;

  EventType recentEvents[kMaxRecentEvents] = {
      EventType::None, EventType::None, EventType::None, EventType::None,
      EventType::None, EventType::None, EventType::None, EventType::None};
  uint8_t nextIndex = 0;
  uint8_t validCount = 0;

  void push(EventType type) {
    recentEvents[nextIndex] = type;
    nextIndex = static_cast<uint8_t>((nextIndex + 1) % kMaxRecentEvents);
    if (validCount < kMaxRecentEvents) {
      validCount++;
    }
  }
};
