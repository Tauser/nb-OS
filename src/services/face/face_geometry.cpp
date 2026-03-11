#include "face_geometry.h"

#include "../../utils/math_utils.h"

namespace {
FaceShapeProfile makeNeutralProfile(float legacyRoundness) {
  FaceShapeProfile p;
  p.base.mode = EyeShapeMode::RoundedRect;
  p.base.topWidthScale = 1.00f;
  p.base.bottomWidthScale = 1.00f;
  p.base.innerCantDeg = 0.0f;
  p.base.outerCantDeg = 0.0f;
  p.base.roundTop = legacyRoundness;
  p.base.roundBottom = legacyRoundness;
  p.base.useCornerRoundness = false;
  p.sanitize();
  return p;
}

void tuneByExpression(FaceShapeProfile& p, ExpressionType expression, const EmotionState& emo) {
  switch (expression) {
    case ExpressionType::Curiosity:
      p.base.mode = EyeShapeMode::Wedge;
      p.base.topWidthScale = 1.06f;
      p.base.bottomWidthScale = 0.82f;
      p.base.innerCantDeg = 8.0f;
      p.base.outerCantDeg = -6.0f;
      p.base.roundTop = 0.42f;
      p.base.roundBottom = 0.30f;
      p.right.enabled = true;
      p.right.style = p.base;
      p.right.style.topWidthScale = 1.10f;
      p.right.style.innerCantDeg += 2.0f;
      break;

    case ExpressionType::FaceRecognized:
      p.base.mode = EyeShapeMode::SoftCapsule;
      p.base.topWidthScale = 1.02f;
      p.base.bottomWidthScale = 1.00f;
      p.base.innerCantDeg = 1.0f;
      p.base.outerCantDeg = -1.0f;
      p.base.roundTop = 0.82f;
      p.base.roundBottom = 0.78f;
      break;

    case ExpressionType::BatteryAlert:
      p.base.mode = EyeShapeMode::Trapezoid;
      p.base.topWidthScale = 0.82f;
      p.base.bottomWidthScale = 1.10f;
      p.base.innerCantDeg = -4.0f;
      p.base.outerCantDeg = 4.0f;
      p.base.roundTop = 0.32f;
      p.base.roundBottom = 0.20f;
      break;

    case ExpressionType::Angry:
      p.base.mode = EyeShapeMode::Wedge;
      p.base.topWidthScale = 1.08f;
      p.base.bottomWidthScale = 0.72f;
      p.base.innerCantDeg = 14.0f;
      p.base.outerCantDeg = -10.0f;
      p.base.roundTop = 0.28f;
      p.base.roundBottom = 0.14f;
      p.base.useCornerRoundness = true;
      p.base.roundTopInner = 0.06f;
      p.base.roundTopOuter = 0.35f;
      p.base.roundBottomInner = 0.10f;
      p.base.roundBottomOuter = 0.24f;
      break;

    case ExpressionType::Sad:
      p.base.mode = EyeShapeMode::Trapezoid;
      p.base.topWidthScale = 1.08f;
      p.base.bottomWidthScale = 0.86f;
      p.base.innerCantDeg = -8.0f;
      p.base.outerCantDeg = 6.0f;
      p.base.roundTop = 0.62f;
      p.base.roundBottom = 0.40f;
      break;

    case ExpressionType::Surprised:
      p.base.mode = EyeShapeMode::SoftCapsule;
      p.base.topWidthScale = 1.16f;
      p.base.bottomWidthScale = 1.10f;
      p.base.innerCantDeg = 0.0f;
      p.base.outerCantDeg = 0.0f;
      p.base.roundTop = 0.92f;
      p.base.roundBottom = 0.88f;
      break;

    case ExpressionType::Shy:
      p.base.mode = EyeShapeMode::RoundedRect;
      p.base.topWidthScale = 0.95f;
      p.base.bottomWidthScale = 1.02f;
      p.base.innerCantDeg = -5.0f;
      p.base.outerCantDeg = 2.0f;
      p.base.roundTop = 0.72f;
      p.base.roundBottom = 0.58f;
      p.left.enabled = true;
      p.left.style = p.base;
      p.left.style.topWidthScale = 0.88f;
      p.left.style.bottomWidthScale = 0.96f;
      break;

    case ExpressionType::Listening:
      p.base.mode = EyeShapeMode::Trapezoid;
      p.base.topWidthScale = 0.92f;
      p.base.bottomWidthScale = 1.08f;
      p.base.innerCantDeg = 4.0f;
      p.base.outerCantDeg = -2.0f;
      p.base.roundTop = 0.44f;
      p.base.roundBottom = 0.30f;
      break;

    case ExpressionType::Thinking:
      p.base.mode = EyeShapeMode::Wedge;
      p.base.topWidthScale = 0.96f;
      p.base.bottomWidthScale = 0.86f;
      p.base.innerCantDeg = 10.0f;
      p.base.outerCantDeg = -4.0f;
      p.base.roundTop = 0.40f;
      p.base.roundBottom = 0.28f;
      p.left.enabled = true;
      p.left.style = p.base;
      p.left.style.innerCantDeg += 3.0f;
      break;

    case ExpressionType::Blink:
      p.base.mode = EyeShapeMode::SoftCapsule;
      p.base.topWidthScale = 1.0f;
      p.base.bottomWidthScale = 1.0f;
      p.base.innerCantDeg = 0.0f;
      p.base.outerCantDeg = 0.0f;
      p.base.roundTop = 0.86f;
      p.base.roundBottom = 0.84f;
      break;

    case ExpressionType::Neutral:
    default:
      break;
  }

  // Subtle emotion-aware refinement without changing ownership.
  p.base.topWidthScale = MathUtils::clamp(p.base.topWidthScale + (emo.curiosity - 0.5f) * 0.06f, 0.42f, 1.35f);
  p.base.bottomWidthScale = MathUtils::clamp(p.base.bottomWidthScale - (1.0f - emo.energy) * 0.04f, 0.42f, 1.35f);
}

FaceEyeShapeStyle styleForSide(const FaceShapeProfile& profile, EyeSide side) {
  if (side == EyeSide::Left && profile.left.enabled) {
    return profile.left.style;
  }
  if (side == EyeSide::Right && profile.right.enabled) {
    return profile.right.style;
  }
  return profile.base;
}
}

FaceShapeProfile FaceGeometry::profileForExpression(ExpressionType expression,
                                                    const EmotionState& emo,
                                                    float legacyRoundness) {
  FaceShapeProfile profile = makeNeutralProfile(MathUtils::clamp(legacyRoundness, 0.2f, 0.9f));
  tuneByExpression(profile, expression, emo);
  profile.sanitize();
  return profile;
}

ResolvedEyeShape FaceGeometry::resolveEyeShape(const FaceShapeProfile& profile, EyeSide side) {
  const FaceEyeShapeStyle style = styleForSide(profile, side);

  ResolvedEyeShape out;
  out.mode = style.mode;
  out.topWidthScale = style.topWidthScale;
  out.bottomWidthScale = style.bottomWidthScale;

  // Inner/outer are semantic in face space; resolve to left/right local edges.
  const bool isLeftEye = (side == EyeSide::Left);
  out.leftCantDeg = isLeftEye ? style.outerCantDeg : style.innerCantDeg;
  out.rightCantDeg = isLeftEye ? style.innerCantDeg : style.outerCantDeg;

  if (style.useCornerRoundness) {
    out.roundTopLeft = isLeftEye ? style.roundTopOuter : style.roundTopInner;
    out.roundTopRight = isLeftEye ? style.roundTopInner : style.roundTopOuter;
    out.roundBottomLeft = isLeftEye ? style.roundBottomOuter : style.roundBottomInner;
    out.roundBottomRight = isLeftEye ? style.roundBottomInner : style.roundBottomOuter;
  } else {
    out.roundTopLeft = style.roundTop;
    out.roundTopRight = style.roundTop;
    out.roundBottomLeft = style.roundBottom;
    out.roundBottomRight = style.roundBottom;
  }

  out.sanitize();
  return out;
}
