#pragma once

struct EmotionState {
  // valence: negative to positive affective tone [-1.0, 1.0]
  float valence = 0.0f;
  // Remaining dimensions are normalized [0.0, 1.0]
  float arousal = 0.35f;
  float curiosity = 0.40f;
  float attention = 0.50f;
  float bond = 0.30f;
  float energy = 0.70f;

  static EmotionState neutral();
  void normalize();

  static constexpr float kValenceMin = -1.0f;
  static constexpr float kValenceMax = 1.0f;
  static constexpr float kUnitMin = 0.0f;
  static constexpr float kUnitMax = 1.0f;
};
