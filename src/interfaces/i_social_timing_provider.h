#pragma once

#include "../models/social_timing_state.h"

class ISocialTimingProvider {
public:
  virtual ~ISocialTimingProvider() = default;
  virtual const SocialTimingState& getState() const = 0;
};
