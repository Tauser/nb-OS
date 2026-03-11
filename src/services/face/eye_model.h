#pragma once

#include "../../models/face_shape_profile.h"
#include "expression_types.h"

struct EyeModel {
  int centerX = 0;
  int centerY = 0;
  int eyeRadius = 30;

  float openness = 1.0f;
  float upperLid = 0.18f;
  float lowerLid = 0.10f;
  float tiltDeg = 0.0f;
  float squashY = 1.0f;
  float stretchX = 1.0f;
  float roundness = 0.52f;

  // Geometry V2. Legacy path remains available through roundness/tilt/lids.
  bool useShapeV2 = false;
  ResolvedEyeShape shape{};

  ExpressionType expression = ExpressionType::Neutral;
};
