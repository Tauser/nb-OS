#pragma once
#include "../core/event_types.h"
#include <stdint.h>

struct Event {
  EventType type = EventType::None;
  int source = 0;
  int value = 0;
  uint32_t timestamp = 0;
};
