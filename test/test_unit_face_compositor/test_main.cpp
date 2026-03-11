#include <unity.h>

#include "../../src/services/face/face_compositor.cpp"

void test_base_expression_respects_hold_and_transition_cooldown() {
  FaceCompositor compositor;
  compositor.init(0);

  FaceRenderState proposed;
  proposed.base.expression = ExpressionType::Curiosity;

  FaceRenderState at500 = compositor.resolve(proposed, 500);
  TEST_ASSERT_EQUAL(ExpressionType::Curiosity, at500.base.expression);
  TEST_ASSERT_TRUE(at500.base.holdUntilMs >= 1200);

  proposed.base.expression = ExpressionType::Neutral;
  FaceRenderState at600 = compositor.resolve(proposed, 600);
  TEST_ASSERT_EQUAL(ExpressionType::Curiosity, at600.base.expression);

  FaceRenderState at1300 = compositor.resolve(proposed, 1300);
  TEST_ASSERT_EQUAL(ExpressionType::Neutral, at1300.base.expression);
}

void test_clip_ownership_prevents_gaze_and_blink_conflicts() {
  FaceCompositor compositor;
  compositor.init(0);

  FaceRenderState proposed;
  proposed.gaze.enabled = true;
  proposed.gaze.mode = FaceGazeMode::Hold;
  proposed.gaze.targetXNorm = 0.4f;
  proposed.blink.active = true;
  proposed.blink.mode = FaceBlinkMode::Auto;
  proposed.idle.enabled = true;

  proposed.clip.active = true;
  proposed.clip.kind = FaceClipKind::ThinkingLoop;
  proposed.clip.startedAtMs = 500;
  proposed.clip.durationMs = 1000;
  proposed.clip.ownsGaze = true;
  proposed.clip.allowsBlink = false;

  FaceRenderState resolved = compositor.resolve(proposed, 500);

  TEST_ASSERT_TRUE(resolved.clip.active);
  TEST_ASSERT_EQUAL(FaceClipKind::ThinkingLoop, resolved.clip.kind);

  TEST_ASSERT_FALSE(resolved.gaze.enabled);
  TEST_ASSERT_EQUAL(FaceGazeMode::None, resolved.gaze.mode);
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, resolved.gaze.targetXNorm);

  TEST_ASSERT_FALSE(resolved.blink.active);
  TEST_ASSERT_EQUAL(FaceBlinkMode::Suppressed, resolved.blink.mode);

  TEST_ASSERT_FALSE(resolved.idle.enabled);
}

void test_transient_reaction_has_antispam_and_clip_block() {
  FaceCompositor compositor;
  compositor.init(0);

  FaceRenderState proposed;
  proposed.transient.active = true;
  proposed.transient.kind = FaceTransientReactionKind::Notice;
  proposed.transient.startedAtMs = 500;
  proposed.transient.durationMs = 30;

  FaceRenderState first = compositor.resolve(proposed, 500);
  TEST_ASSERT_TRUE(first.transient.active);
  TEST_ASSERT_EQUAL(FaceTransientReactionKind::Notice, first.transient.kind);
  TEST_ASSERT_TRUE(first.transient.durationMs >= compositor.policy().transientMinDurationMs);

  proposed.transient.kind = FaceTransientReactionKind::Startle;
  proposed.transient.startedAtMs = 600;
  proposed.transient.durationMs = 400;
  FaceRenderState cooldownBlocked = compositor.resolve(proposed, 600);
  TEST_ASSERT_EQUAL(FaceTransientReactionKind::Notice, cooldownBlocked.transient.kind);

  FaceRenderState acceptedLater = compositor.resolve(proposed, 1400);
  TEST_ASSERT_EQUAL(FaceTransientReactionKind::Startle, acceptedLater.transient.kind);

  proposed.clip.active = true;
  proposed.clip.kind = FaceClipKind::AttentionRecovery;
  proposed.clip.startedAtMs = 1500;
  proposed.clip.durationMs = 800;
  proposed.transient.kind = FaceTransientReactionKind::Recovery;
  proposed.transient.startedAtMs = 1500;
  proposed.transient.durationMs = 260;

  FaceRenderState blockedByClip = compositor.resolve(proposed, 1500);
  TEST_ASSERT_EQUAL(FaceTransientReactionKind::Startle, blockedByClip.transient.kind);
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_base_expression_respects_hold_and_transition_cooldown);
  RUN_TEST(test_clip_ownership_prevents_gaze_and_blink_conflicts);
  RUN_TEST(test_transient_reaction_has_antispam_and_clip_block);
  UNITY_END();
}

void loop() {}
