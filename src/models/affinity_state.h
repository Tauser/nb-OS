#pragma once

struct AffinityState {
  float bond = 0.25f;

  void clamp() {
    if (bond < 0.0f) bond = 0.0f;
    if (bond > 1.0f) bond = 1.0f;
  }
};
