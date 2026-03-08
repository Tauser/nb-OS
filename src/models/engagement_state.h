#pragma once

struct EngagementState {
  float score = 0.0f;

  void clamp() {
    if (score < 0.0f) score = 0.0f;
    if (score > 1.0f) score = 1.0f;
  }
};
