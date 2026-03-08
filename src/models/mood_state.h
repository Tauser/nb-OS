#pragma once

#include <stdint.h>

struct MoodState {
  float valence = 0.0f;
  float stability = 0.5f;

  void clamp() {
    if (valence < -1.0f) valence = -1.0f;
    if (valence > 1.0f) valence = 1.0f;
    if (stability < 0.0f) stability = 0.0f;
    if (stability > 1.0f) stability = 1.0f;
  }
};
