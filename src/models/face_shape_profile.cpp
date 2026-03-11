#include "face_shape_profile.h"

#include "../utils/math_utils.h"

namespace {
float clampWidth(float v) {
  return MathUtils::clamp(v, 0.42f, 1.35f);
}

float clampCant(float v) {
  return MathUtils::clamp(v, -35.0f, 35.0f);
}

float clampRound(float v) {
  return MathUtils::clamp(v, 0.0f, 1.0f);
}
}

void FaceEyeShapeStyle::sanitize() {
  topWidthScale = clampWidth(topWidthScale);
  bottomWidthScale = clampWidth(bottomWidthScale);

  innerCantDeg = clampCant(innerCantDeg);
  outerCantDeg = clampCant(outerCantDeg);

  roundTop = clampRound(roundTop);
  roundBottom = clampRound(roundBottom);

  roundTopInner = clampRound(roundTopInner);
  roundTopOuter = clampRound(roundTopOuter);
  roundBottomInner = clampRound(roundBottomInner);
  roundBottomOuter = clampRound(roundBottomOuter);
}

void FaceEyeShapeOverride::sanitize() {
  style.sanitize();
}

void FaceShapeProfile::sanitize() {
  base.sanitize();
  left.sanitize();
  right.sanitize();
}

void ResolvedEyeShape::sanitize() {
  topWidthScale = clampWidth(topWidthScale);
  bottomWidthScale = clampWidth(bottomWidthScale);
  leftCantDeg = clampCant(leftCantDeg);
  rightCantDeg = clampCant(rightCantDeg);
  roundTopLeft = clampRound(roundTopLeft);
  roundTopRight = clampRound(roundTopRight);
  roundBottomLeft = clampRound(roundBottomLeft);
  roundBottomRight = clampRound(roundBottomRight);
}

float faceShapeFallbackRoundness(const ResolvedEyeShape& shape) {
  const float sum = shape.roundTopLeft + shape.roundTopRight + shape.roundBottomLeft + shape.roundBottomRight;
  return MathUtils::clamp(sum * 0.25f, 0.2f, 0.9f);
}
