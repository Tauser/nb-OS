#include <unity.h>

#include "../../src/services/face/expression_preset.cpp"

void test_preset_library_is_frozen_and_validated() {
  TEST_ASSERT_TRUE(facePresetLibraryFrozen());

  const char* reason = nullptr;
  TEST_ASSERT_TRUE(validateFacePresetLibrary(&reason));
  TEST_ASSERT_NULL(reason);
}

void test_official_presets_have_expected_tiers_and_non_exploratory_status() {
  struct Expected {
    FacePresetId id;
    const char* name;
    FacePresetReadabilityTier tier;
  };

  const Expected expected[] = {
      {FacePresetId::Neutral, "neutral_premium", FacePresetReadabilityTier::TierC},
      {FacePresetId::Curious, "curious_soft", FacePresetReadabilityTier::TierC},
      {FacePresetId::Sleepy, "sleepy_sink", FacePresetReadabilityTier::TierA},
      {FacePresetId::HappySoft, "warm_happy", FacePresetReadabilityTier::TierC},
      {FacePresetId::Focused, "focused_listen", FacePresetReadabilityTier::TierB},
      {FacePresetId::Shy, "shy_peek", FacePresetReadabilityTier::TierB},
      {FacePresetId::Surprised, "surprised_open", FacePresetReadabilityTier::TierA},
      {FacePresetId::LowEnergy, "low_energy_flat", FacePresetReadabilityTier::TierA},
      {FacePresetId::Listening, "attention_lock", FacePresetReadabilityTier::TierA},
      {FacePresetId::Thinking, "thinking_side", FacePresetReadabilityTier::TierB},
  };

  for (size_t i = 0; i < (sizeof(expected) / sizeof(expected[0])); ++i) {
    const ExpressionPreset& preset = getExpressionPreset(expected[i].id);
    TEST_ASSERT_EQUAL_STRING(expected[i].name, preset.name);
    TEST_ASSERT_EQUAL(expected[i].tier, preset.readabilityTier);
    TEST_ASSERT_FALSE(preset.explorationOnly);
  }
}

void test_layer_blend_and_tuning_guardrails_are_consistent() {
  size_t count = 0;
  const ExpressionPreset* presets = facePresetLibrary(&count);
  TEST_ASSERT_NOT_NULL(presets);
  TEST_ASSERT_EQUAL(10, static_cast<int>(count));

  for (size_t i = 0; i < count; ++i) {
    const ExpressionPreset& preset = presets[i];

    TEST_ASSERT_TRUE(preset.layerBlend.moodGain > 0.5f);
    TEST_ASSERT_TRUE(preset.layerBlend.clipGain > 0.5f);
    TEST_ASSERT_TRUE(preset.layerBlend.blinkGain > 0.5f);

    TEST_ASSERT_TRUE(preset.tuning.widthMin < preset.tuning.widthMax);
    TEST_ASSERT_TRUE(preset.tuning.heightMin < preset.tuning.heightMax);
    TEST_ASSERT_TRUE(preset.widthScale >= preset.tuning.widthMin);
    TEST_ASSERT_TRUE(preset.widthScale <= preset.tuning.widthMax);
    TEST_ASSERT_TRUE(preset.heightScale >= preset.tuning.heightMin);
    TEST_ASSERT_TRUE(preset.heightScale <= preset.tuning.heightMax);
  }
}

void test_choose_preset_maps_expression_and_state_without_hacks() {
  EmotionState e{};
  e.energy = 0.7f;
  e.attention = 0.5f;
  e.curiosity = 0.4f;
  e.arousal = 0.3f;

  TEST_ASSERT_EQUAL(FacePresetId::Listening, choosePreset(ExpressionType::Listening, e));
  TEST_ASSERT_EQUAL(FacePresetId::Thinking, choosePreset(ExpressionType::Thinking, e));
  TEST_ASSERT_EQUAL(FacePresetId::Surprised, choosePreset(ExpressionType::Surprised, e));

  e.energy = 0.18f;
  TEST_ASSERT_EQUAL(FacePresetId::LowEnergy, choosePreset(ExpressionType::Neutral, e));
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_preset_library_is_frozen_and_validated);
  RUN_TEST(test_official_presets_have_expected_tiers_and_non_exploratory_status);
  RUN_TEST(test_layer_blend_and_tuning_guardrails_are_consistent);
  RUN_TEST(test_choose_preset_maps_expression_and_state_without_hacks);
  UNITY_END();
}

void loop() {}
