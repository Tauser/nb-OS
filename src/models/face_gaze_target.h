#pragma once

#include <stdint.h>

enum class FaceSaccadeSize : uint8_t {
  Auto = 0,
  Short,
  Medium,
  Long
};

enum class FaceGazeBehavior : uint8_t {
  Hold = 0,
  EdgePeek,
  RecoveryGlance,
  Scan
};

struct FaceGazeTarget {
  bool enabled = true;
  float xNorm = 0.0f;
  float yNorm = 0.0f;
  FaceSaccadeSize saccadeSize = FaceSaccadeSize::Auto;
  FaceGazeBehavior behavior = FaceGazeBehavior::Hold;
  unsigned long fixationMs = 0;
  bool microSaccades = false;
  float weight = 1.0f;

  void sanitize();

  static FaceGazeTarget center();
};
