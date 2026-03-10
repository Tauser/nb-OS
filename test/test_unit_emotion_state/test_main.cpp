#include <unity.h>

#include "../../src/models/emotion_state.h"
#include "../../src/models/emotion_state.cpp"

void test_emotion_state_normalize_clamps_ranges() {
  EmotionState s{};
  s.valence = 3.0f;
  s.arousal = -0.7f;
  s.curiosity = 2.4f;
  s.attention = -1.1f;
  s.bond = 4.2f;
  s.energy = -8.0f;

  s.normalize();

  TEST_ASSERT_FLOAT_WITHIN(0.0001f, EmotionState::kValenceMax, s.valence);
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, EmotionState::kUnitMin, s.arousal);
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, EmotionState::kUnitMax, s.curiosity);
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, EmotionState::kUnitMin, s.attention);
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, EmotionState::kUnitMax, s.bond);
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, EmotionState::kUnitMin, s.energy);
}

void test_emotion_state_neutral_is_normalized() {
  EmotionState s = EmotionState::neutral();
  TEST_ASSERT_TRUE(s.valence >= EmotionState::kValenceMin && s.valence <= EmotionState::kValenceMax);
  TEST_ASSERT_TRUE(s.arousal >= EmotionState::kUnitMin && s.arousal <= EmotionState::kUnitMax);
  TEST_ASSERT_TRUE(s.curiosity >= EmotionState::kUnitMin && s.curiosity <= EmotionState::kUnitMax);
  TEST_ASSERT_TRUE(s.attention >= EmotionState::kUnitMin && s.attention <= EmotionState::kUnitMax);
  TEST_ASSERT_TRUE(s.bond >= EmotionState::kUnitMin && s.bond <= EmotionState::kUnitMax);
  TEST_ASSERT_TRUE(s.energy >= EmotionState::kUnitMin && s.energy <= EmotionState::kUnitMax);
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_emotion_state_normalize_clamps_ranges);
  RUN_TEST(test_emotion_state_neutral_is_normalized);
  UNITY_END();
}

void loop() {
}
