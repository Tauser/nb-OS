#pragma once

#include "../../models/emotion_state.h"
#include "../../models/expression_types.h"
#include "../../models/face_shape_profile.h"

class FaceGeometry {
public:
  static FaceShapeProfile profileForExpression(ExpressionType expression,
                                               const EmotionState& emo,
                                               float legacyRoundness);

  static ResolvedEyeShape resolveEyeShape(const FaceShapeProfile& profile, EyeSide side);
};
