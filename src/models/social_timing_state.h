#pragma once

struct SocialTimingState {
  float responsiveness = 0.5f;
  float initiative = 0.5f;
  float persistence = 0.5f;
  float pauseFactor = 0.5f;

  void clamp() {
    if (responsiveness < 0.0f) responsiveness = 0.0f;
    if (responsiveness > 1.0f) responsiveness = 1.0f;
    if (initiative < 0.0f) initiative = 0.0f;
    if (initiative > 1.0f) initiative = 1.0f;
    if (persistence < 0.0f) persistence = 0.0f;
    if (persistence > 1.0f) persistence = 1.0f;
    if (pauseFactor < 0.0f) pauseFactor = 0.0f;
    if (pauseFactor > 1.0f) pauseFactor = 1.0f;
  }
};
