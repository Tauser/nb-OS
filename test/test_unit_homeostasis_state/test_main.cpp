#include <unity.h>

#include "../../src/models/homeostasis_state.h"

void test_homeostasis_state_clamp_limits_ranges() {
  HomeostasisState s{};
  s.energy = 2.0f;
  s.stimulation = -0.4f;
  s.curiosity = 2.1f;
  s.sociabilityBias = -1.0f;
  s.sensitivityBias = 3.0f;
  s.affinityBias = -0.1f;
  s.noveltyBias = 4.7f;

  s.scores.calm = 1.5f;
  s.scores.animated = -0.5f;
  s.scores.curious = 2.3f;
  s.scores.social = -2.0f;
  s.scores.sensitive = 9.0f;
  s.scores.sleepy = -1.2f;
  s.scores.bored = 4.0f;

  s.clamp();

  TEST_ASSERT_TRUE(s.energy >= 0.0f && s.energy <= 1.0f);
  TEST_ASSERT_TRUE(s.stimulation >= 0.0f && s.stimulation <= 1.0f);
  TEST_ASSERT_TRUE(s.curiosity >= 0.0f && s.curiosity <= 1.0f);
  TEST_ASSERT_TRUE(s.sociabilityBias >= 0.0f && s.sociabilityBias <= 1.0f);
  TEST_ASSERT_TRUE(s.sensitivityBias >= 0.0f && s.sensitivityBias <= 1.0f);
  TEST_ASSERT_TRUE(s.affinityBias >= 0.0f && s.affinityBias <= 1.0f);
  TEST_ASSERT_TRUE(s.noveltyBias >= 0.0f && s.noveltyBias <= 1.0f);
  TEST_ASSERT_TRUE(s.scores.calm >= 0.0f && s.scores.calm <= 1.0f);
  TEST_ASSERT_TRUE(s.scores.animated >= 0.0f && s.scores.animated <= 1.0f);
  TEST_ASSERT_TRUE(s.scores.curious >= 0.0f && s.scores.curious <= 1.0f);
  TEST_ASSERT_TRUE(s.scores.social >= 0.0f && s.scores.social <= 1.0f);
  TEST_ASSERT_TRUE(s.scores.sensitive >= 0.0f && s.scores.sensitive <= 1.0f);
  TEST_ASSERT_TRUE(s.scores.sleepy >= 0.0f && s.scores.sleepy <= 1.0f);
  TEST_ASSERT_TRUE(s.scores.bored >= 0.0f && s.scores.bored <= 1.0f);
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_homeostasis_state_clamp_limits_ranges);
  UNITY_END();
}

void loop() {
}
