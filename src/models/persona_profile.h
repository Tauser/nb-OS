#pragma once

#include <stdint.h>

enum class PersonaTone : uint8_t {
  Warm = 0,
  Playful,
  Calm
};

enum class PersonaResponseStyle : uint8_t {
  Balanced = 0,
  Gentle,
  Energetic,
  Reserved
};

struct PersonaProfile {
  PersonaTone tone = PersonaTone::Warm;
  PersonaTone baseTone = PersonaTone::Warm;

  float expressiveness = 0.5f;
  float sociability = 0.5f;
  float initiative = 0.5f;
  float intensity = 0.5f;
  float socialProximity = 0.5f;

  PersonaResponseStyle responseStyle = PersonaResponseStyle::Balanced;

  void clamp() {
    if (expressiveness < 0.0f) expressiveness = 0.0f;
    if (expressiveness > 1.0f) expressiveness = 1.0f;
    if (sociability < 0.0f) sociability = 0.0f;
    if (sociability > 1.0f) sociability = 1.0f;
    if (initiative < 0.0f) initiative = 0.0f;
    if (initiative > 1.0f) initiative = 1.0f;
    if (intensity < 0.0f) intensity = 0.0f;
    if (intensity > 1.0f) intensity = 1.0f;
    if (socialProximity < 0.0f) socialProximity = 0.0f;
    if (socialProximity > 1.0f) socialProximity = 1.0f;
  }
};
