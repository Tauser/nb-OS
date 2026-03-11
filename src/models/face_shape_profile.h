#pragma once

#include <stdint.h>

enum class EyeSide : uint8_t {
  Left = 0,
  Right
};

enum class EyeShapeMode : uint8_t {
  RoundedRect = 0,
  Wedge,
  Trapezoid,
  SoftCapsule
};

// Semantic shape controls. Inner/outer cant are defined in face space and are
// resolved per-eye by FaceGeometry.
struct FaceEyeShapeStyle {
  EyeShapeMode mode = EyeShapeMode::RoundedRect;

  float topWidthScale = 1.0f;
  float bottomWidthScale = 1.0f;

  float innerCantDeg = 0.0f;
  float outerCantDeg = 0.0f;

  // Pair-based roundness.
  float roundTop = 0.52f;
  float roundBottom = 0.52f;

  // Optional corner-specific roundness (inner/outer semantics).
  bool useCornerRoundness = false;
  float roundTopInner = 0.52f;
  float roundTopOuter = 0.52f;
  float roundBottomInner = 0.52f;
  float roundBottomOuter = 0.52f;

  void sanitize();
};

struct FaceEyeShapeOverride {
  bool enabled = false;
  FaceEyeShapeStyle style{};

  void sanitize();
};

struct FaceShapeProfile {
  FaceEyeShapeStyle base{};
  FaceEyeShapeOverride left{};
  FaceEyeShapeOverride right{};

  void sanitize();
};

// Renderer-facing resolved shape uses left/right edges in eye-local space.
struct ResolvedEyeShape {
  EyeShapeMode mode = EyeShapeMode::RoundedRect;

  float topWidthScale = 1.0f;
  float bottomWidthScale = 1.0f;

  float leftCantDeg = 0.0f;
  float rightCantDeg = 0.0f;

  float roundTopLeft = 0.52f;
  float roundTopRight = 0.52f;
  float roundBottomLeft = 0.52f;
  float roundBottomRight = 0.52f;

  void sanitize();
};

float faceShapeFallbackRoundness(const ResolvedEyeShape& shape);
