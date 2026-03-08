#pragma once

#include <stdint.h>

enum class MoodProfile : uint8_t {
  Calm = 0,
  Animated,
  Sleepy,
  Curious,
  Sensitive,
  Social,
  Reserved
};
