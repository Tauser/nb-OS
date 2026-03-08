#pragma once

#include "mood_profile.h"
#include <stdint.h>

struct MoodState {
  float valence = 0.0f;
  float stability = 0.5f;
  float socialDrive = 0.5f;
  float reserve = 0.5f;
  float energyTrend = 0.6f;
  float curiosityTrend = 0.5f;
  MoodProfile profile = MoodProfile::Calm;

  void clamp() {
    if (valence < -1.0f) valence = -1.0f;
    if (valence > 1.0f) valence = 1.0f;
    if (stability < 0.0f) stability = 0.0f;
    if (stability > 1.0f) stability = 1.0f;
    if (socialDrive < 0.0f) socialDrive = 0.0f;
    if (socialDrive > 1.0f) socialDrive = 1.0f;
    if (reserve < 0.0f) reserve = 0.0f;
    if (reserve > 1.0f) reserve = 1.0f;
    if (energyTrend < 0.0f) energyTrend = 0.0f;
    if (energyTrend > 1.0f) energyTrend = 1.0f;
    if (curiosityTrend < 0.0f) curiosityTrend = 0.0f;
    if (curiosityTrend > 1.0f) curiosityTrend = 1.0f;
  }
};
