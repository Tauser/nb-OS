#include "companion_state.h"

#include "../utils/math_utils.h"

void CompanionState::clamp() {
  moodValence = MathUtils::clamp(moodValence, -1.0f, 1.0f);
  engagement = MathUtils::clamp(engagement, 0.0f, 1.0f);
  affinityBond = MathUtils::clamp(affinityBond, 0.0f, 1.0f);
  socialResponsiveness = MathUtils::clamp(socialResponsiveness, 0.0f, 1.0f);
  socialInitiative = MathUtils::clamp(socialInitiative, 0.0f, 1.0f);
  socialPersistence = MathUtils::clamp(socialPersistence, 0.0f, 1.0f);
  socialPauseFactor = MathUtils::clamp(socialPauseFactor, 0.0f, 1.0f);
  socialReadiness = MathUtils::clamp(socialReadiness, 0.0f, 1.0f);
  initiativeReadiness = MathUtils::clamp(initiativeReadiness, 0.0f, 1.0f);
}

void CompanionState::refreshDerived() {
  const float moodNorm = MathUtils::clamp((moodValence + 1.0f) * 0.5f, 0.0f, 1.0f);
  socialReadiness = (engagement * 0.34f) + (affinityBond * 0.26f) +
                    (socialResponsiveness * 0.22f) + (moodNorm * 0.18f);

  initiativeReadiness = (socialInitiative * 0.45f) + (socialPersistence * 0.20f) +
                        (engagement * 0.20f) + ((1.0f - socialPauseFactor) * 0.15f);

  clamp();
}
