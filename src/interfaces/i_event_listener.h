#pragma once
#include "../models/event.h"

class IEventListener {
public:
  virtual ~IEventListener() = default;
  virtual void onEvent(const Event& event) = 0;
};
