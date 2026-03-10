#pragma once

#include "../models/companion_state.h"

class ICompanionStateProvider {
public:
  virtual ~ICompanionStateProvider() = default;
  virtual const CompanionState& getState() const = 0;
};
