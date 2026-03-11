#include <unity.h>

#include "../../src/models/face_clip.cpp"
#include "../../src/services/face/face_clip_player.cpp"

void test_clip_player_supports_preemption() {
  FaceClipPlayer player;
  player.init(0);

  TEST_ASSERT_TRUE(player.play(FaceClipKind::WakeUp, 0, true));
  player.update(120);
  TEST_ASSERT_TRUE(player.isActive());
  TEST_ASSERT_EQUAL(FaceClipKind::WakeUp, player.activeKind());

  TEST_ASSERT_TRUE(player.play(FaceClipKind::HappyAck, 120, true));
  player.update(140);

  TEST_ASSERT_TRUE(player.isActive());
  TEST_ASSERT_EQUAL(FaceClipKind::HappyAck, player.activeKind());
  TEST_ASSERT_TRUE(player.state().active);
}

void test_clip_player_supports_cancel_and_clears_state() {
  FaceClipPlayer player;
  player.init(0);

  TEST_ASSERT_TRUE(player.play(FaceClipKind::ThinkingLoop, 0, true));
  player.update(220);
  TEST_ASSERT_TRUE(player.isActive());

  TEST_ASSERT_TRUE(player.cancel(220));
  player.update(250);

  TEST_ASSERT_FALSE(player.isActive());
  TEST_ASSERT_EQUAL(FaceClipKind::None, player.state().kind);
  TEST_ASSERT_EQUAL(0, player.sample().offsetX);
  TEST_ASSERT_EQUAL(0, player.sample().offsetY);
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, player.sample().opennessDelta);
}

void test_clip_player_returns_to_baseline_after_completion() {
  FaceClipPlayer player;
  player.init(0);

  TEST_ASSERT_TRUE(player.play(FaceClipKind::AttentionRecovery, 0, true));

  for (unsigned long nowMs = 0; nowMs <= 900; nowMs += 90) {
    player.update(nowMs);
  }

  TEST_ASSERT_FALSE(player.isActive());
  TEST_ASSERT_EQUAL(FaceClipKind::None, player.state().kind);
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, player.sample().tiltDegDelta);
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, player.sample().upperLidDelta);
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_clip_player_supports_preemption);
  RUN_TEST(test_clip_player_supports_cancel_and_clears_state);
  RUN_TEST(test_clip_player_returns_to_baseline_after_completion);
  UNITY_END();
}

void loop() {}
