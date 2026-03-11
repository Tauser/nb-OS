#include <unity.h>
#include <math.h>

#include "../../src/models/face_gaze_target.cpp"
#include "../../src/services/face/face_gaze_controller.cpp"

void test_gaze_controller_performs_saccade_and_settle() {
  FaceGazeController controller;
  controller.init(0);
  controller.setContext(ExpressionType::Neutral, 0.5f, 0.5f, true);

  FaceGazeTarget target;
  target.enabled = true;
  target.xNorm = 0.72f;
  target.yNorm = 0.00f;
  target.saccadeSize = FaceSaccadeSize::Medium;
  target.behavior = FaceGazeBehavior::Hold;
  target.fixationMs = 700;
  target.microSaccades = false;

  controller.setTarget(target, 0);

  controller.update(40);
  TEST_ASSERT_EQUAL(FaceGazeMode::Saccade, controller.output().mode);
  TEST_ASSERT_TRUE(controller.output().currentXNorm > 0.10f);

  controller.update(320);
  TEST_ASSERT_EQUAL(FaceGazeMode::Hold, controller.output().mode);
  TEST_ASSERT_FLOAT_WITHIN(0.12f, 0.72f, controller.output().targetXNorm);
}

void test_listening_context_enables_micro_saccades() {
  FaceGazeController controller;
  controller.init(0);
  controller.setContext(ExpressionType::Listening, 0.70f, 0.45f, false);

  FaceGazeTarget target;
  target.enabled = true;
  target.xNorm = 0.02f;
  target.yNorm = -0.10f;
  target.saccadeSize = FaceSaccadeSize::Long;
  target.behavior = FaceGazeBehavior::Hold;
  target.fixationMs = 1200;
  target.microSaccades = true;

  controller.setTarget(target, 0);

  bool sawMicro = false;
  for (unsigned long t = 0; t <= 1600; t += 40) {
    controller.setContext(ExpressionType::Listening, 0.70f, 0.45f, false);
    controller.update(t);
    if (controller.output().mode == FaceGazeMode::MicroSaccade) {
      sawMicro = true;
      break;
    }
  }

  TEST_ASSERT_TRUE(sawMicro);
}

void test_edge_peek_recovers_toward_center() {
  FaceGazeController controller;
  controller.init(0);
  controller.setContext(ExpressionType::Curiosity, 0.60f, 0.80f, false);

  FaceGazeTarget target;
  target.enabled = true;
  target.xNorm = 0.84f;
  target.yNorm = -0.04f;
  target.saccadeSize = FaceSaccadeSize::Medium;
  target.behavior = FaceGazeBehavior::EdgePeek;
  target.fixationMs = 260;
  target.microSaccades = false;

  controller.setTarget(target, 0);

  for (unsigned long t = 0; t <= 1300; t += 50) {
    controller.setContext(ExpressionType::Curiosity, 0.60f, 0.80f, false);
    controller.update(t);
  }

  TEST_ASSERT_FLOAT_WITHIN(0.18f, 0.0f, controller.output().currentXNorm);
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_gaze_controller_performs_saccade_and_settle);
  RUN_TEST(test_listening_context_enables_micro_saccades);
  RUN_TEST(test_edge_peek_recovers_toward_center);
  UNITY_END();
}

void loop() {}
