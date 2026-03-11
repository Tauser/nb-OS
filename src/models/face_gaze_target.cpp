#include "face_gaze_target.h"

#include "../utils/math_utils.h"

void FaceGazeTarget::sanitize() {
  xNorm = MathUtils::clamp(xNorm, -1.0f, 1.0f);
  yNorm = MathUtils::clamp(yNorm, -1.0f, 1.0f);
  weight = MathUtils::clamp(weight, 0.0f, 1.0f);
}

FaceGazeTarget FaceGazeTarget::center() {
  FaceGazeTarget out;
  out.enabled = true;
  out.xNorm = 0.0f;
  out.yNorm = 0.0f;
  out.saccadeSize = FaceSaccadeSize::Short;
  out.behavior = FaceGazeBehavior::Hold;
  out.fixationMs = 500;
  out.microSaccades = false;
  out.weight = 1.0f;
  return out;
}
