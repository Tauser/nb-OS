#pragma once

#include <stdint.h>

enum class PersonaTone : uint8_t {
  Warm = 0,
  Playful,
  Calm
};

struct PersonaProfile {
  PersonaTone tone = PersonaTone::Warm;
  float expressiveness = 0.5f;
  float sociability = 0.5f;
};
