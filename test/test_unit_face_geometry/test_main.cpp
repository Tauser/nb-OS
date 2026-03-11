#include <unity.h>
#include <math.h>

#include "../../src/models/face_shape_profile.cpp"
#include "../../src/services/face/face_geometry.cpp"

void test_geometry_profiles_have_clear_mode_differences() {
  EmotionState emo{};
  emo.curiosity = 0.75f;
  emo.energy = 0.80f;

  const FaceShapeProfile neutral = FaceGeometry::profileForExpression(ExpressionType::Neutral, emo, 0.55f);
  const FaceShapeProfile curious = FaceGeometry::profileForExpression(ExpressionType::Curiosity, emo, 0.55f);
  const FaceShapeProfile alert = FaceGeometry::profileForExpression(ExpressionType::BatteryAlert, emo, 0.55f);

  TEST_ASSERT_EQUAL(EyeShapeMode::RoundedRect, neutral.base.mode);
  TEST_ASSERT_EQUAL(EyeShapeMode::Wedge, curious.base.mode);
  TEST_ASSERT_EQUAL(EyeShapeMode::Trapezoid, alert.base.mode);
}

void test_geometry_resolve_applies_inner_outer_by_eye_side() {
  FaceShapeProfile profile{};
  profile.base.mode = EyeShapeMode::Wedge;
  profile.base.innerCantDeg = 12.0f;
  profile.base.outerCantDeg = -6.0f;
  profile.sanitize();

  const ResolvedEyeShape left = FaceGeometry::resolveEyeShape(profile, EyeSide::Left);
  const ResolvedEyeShape right = FaceGeometry::resolveEyeShape(profile, EyeSide::Right);

  TEST_ASSERT_FLOAT_WITHIN(0.001f, -6.0f, left.leftCantDeg);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 12.0f, left.rightCantDeg);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 12.0f, right.leftCantDeg);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, -6.0f, right.rightCantDeg);
}

void test_geometry_supports_per_eye_override() {
  EmotionState emo{};
  emo.curiosity = 0.80f;
  emo.energy = 0.72f;

  const FaceShapeProfile curious = FaceGeometry::profileForExpression(ExpressionType::Curiosity, emo, 0.55f);
  const ResolvedEyeShape left = FaceGeometry::resolveEyeShape(curious, EyeSide::Left);
  const ResolvedEyeShape right = FaceGeometry::resolveEyeShape(curious, EyeSide::Right);

  TEST_ASSERT_TRUE(curious.right.enabled);
  TEST_ASSERT_TRUE(fabsf(left.topWidthScale - right.topWidthScale) > 0.01f);
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_geometry_profiles_have_clear_mode_differences);
  RUN_TEST(test_geometry_resolve_applies_inner_outer_by_eye_side);
  RUN_TEST(test_geometry_supports_per_eye_override);
  UNITY_END();
}

void loop() {
}


