#pragma once

#include "../utils/math_utils.h"
#include <stdint.h>

enum class HomeostasisMode : uint8_t {
  Calm = 0,
  Animated,
  Curious,
  Social,
  Sensitive,
  Sleepy,
  Bored
};

struct HomeostasisScores {
  float calm = 0.0f;
  float animated = 0.0f;
  float curious = 0.0f;
  float social = 0.0f;
  float sensitive = 0.0f;
  float sleepy = 0.0f;
  float bored = 0.0f;

  void clamp() {
    calm = MathUtils::clamp(calm, 0.0f, 1.0f);
    animated = MathUtils::clamp(animated, 0.0f, 1.0f);
    curious = MathUtils::clamp(curious, 0.0f, 1.0f);
    social = MathUtils::clamp(social, 0.0f, 1.0f);
    sensitive = MathUtils::clamp(sensitive, 0.0f, 1.0f);
    sleepy = MathUtils::clamp(sleepy, 0.0f, 1.0f);
    bored = MathUtils::clamp(bored, 0.0f, 1.0f);
  }
};

struct HomeostasisState {
  float energy = 0.70f;
  float stimulation = 0.35f;
  float curiosity = 0.40f;

  float sociabilityBias = 0.45f;
  float sensitivityBias = 0.35f;
  float affinityBias = 0.40f;
  float noveltyBias = 0.45f;

  unsigned long idleMs = 0;
  HomeostasisScores scores{};
  HomeostasisMode mode = HomeostasisMode::Calm;

  void clamp() {
    energy = MathUtils::clamp(energy, 0.0f, 1.0f);
    stimulation = MathUtils::clamp(stimulation, 0.0f, 1.0f);
    curiosity = MathUtils::clamp(curiosity, 0.0f, 1.0f);
    sociabilityBias = MathUtils::clamp(sociabilityBias, 0.0f, 1.0f);
    sensitivityBias = MathUtils::clamp(sensitivityBias, 0.0f, 1.0f);
    affinityBias = MathUtils::clamp(affinityBias, 0.0f, 1.0f);
    noveltyBias = MathUtils::clamp(noveltyBias, 0.0f, 1.0f);
    scores.clamp();
  }
};
