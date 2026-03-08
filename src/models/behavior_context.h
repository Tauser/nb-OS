#pragma once

#include "event.h"

struct BehaviorContext {
  EventType lastEventType = EventType::None;
  unsigned long lastEventMs = 0;
  unsigned long lastInteractionMs = 0;
  unsigned long lastActionMs = 0;
  unsigned long lastIdleEvalMs = 0;
  unsigned long idleTicks = 0;
  bool nextCuriousLeft = true;
  bool nextYawLeft = true;
};
