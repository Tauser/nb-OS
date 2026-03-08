#pragma once

#include <stdint.h>

enum class RoutineState : uint8_t {
  None = 0,
  Idle,
  Resume,
  Charging,
  Listening,
  Rest
};
