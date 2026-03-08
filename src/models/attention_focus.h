#pragma once

#include <stdint.h>

enum class AttentionFocus : uint8_t {
  None = 0,
  Idle,
  Touch,
  Voice,
  Vision,
  Power,
  Internal
};
