#pragma once

#include "../models/action_intent.h"

class IActionOrchestrator {
public:
  virtual ~IActionOrchestrator() = default;
  virtual void init() = 0;
  virtual void update(unsigned long nowMs) = 0;
  virtual bool submitIntent(const ActionIntent& intent, unsigned long nowMs) = 0;
};
