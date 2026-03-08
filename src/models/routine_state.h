#pragma once

#include <stdint.h>

enum class RoutineState : uint8_t {
  None = 0,
  Idle,
  Attentive,
  Calm,
  Curious,
  Sleepy,
  Bored,
  Resume,
  Charging,
  Listening,
  Rest
};
