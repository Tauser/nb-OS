#pragma once

#include "../models/persona_profile.h"

class IPersonaProvider {
public:
  virtual ~IPersonaProvider() = default;
  virtual const PersonaProfile& getProfile() const = 0;
};
