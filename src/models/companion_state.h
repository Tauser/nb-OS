#pragma once

#include "attention_focus.h"
#include "mood_profile.h"
#include "routine_state.h"

struct CompanionState {
  float moodValence = 0.0f;
  MoodProfile moodProfile = MoodProfile::Calm;
  float engagement = 0.0f;
  float affinityBond = 0.25f;

  float socialResponsiveness = 0.5f;
  float socialInitiative = 0.5f;
  float socialPersistence = 0.5f;
  float socialPauseFactor = 0.5f;

  AttentionFocus attentionFocus = AttentionFocus::Idle;
  RoutineState routineState = RoutineState::Idle;

  float socialReadiness = 0.5f;
  float initiativeReadiness = 0.5f;

  unsigned long lastUpdateMs = 0;
  bool valid = false;

  void clamp();
  void refreshDerived();
};
