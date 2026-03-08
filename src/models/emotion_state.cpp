#include "emotion_state.h"

#include "../utils/math_utils.h"

EmotionState EmotionState::neutral() {
  EmotionState state;
  state.normalize();
  return state;
}

void EmotionState::normalize() {
  valence = MathUtils::clamp(valence, kValenceMin, kValenceMax);
  arousal = MathUtils::clamp(arousal, kUnitMin, kUnitMax);
  curiosity = MathUtils::clamp(curiosity, kUnitMin, kUnitMax);
  attention = MathUtils::clamp(attention, kUnitMin, kUnitMax);
  bond = MathUtils::clamp(bond, kUnitMin, kUnitMax);
  energy = MathUtils::clamp(energy, kUnitMin, kUnitMax);
}
