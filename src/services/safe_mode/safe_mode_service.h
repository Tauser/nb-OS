#pragma once

#include "../../core/diagnostics.h"
#include "../../core/event_bus.h"
#include "../../interfaces/i_event_listener.h"
#include "../../interfaces/i_motion.h"

class SafeModeService : public IEventListener {
public:
  SafeModeService(EventBus& eventBus, Diagnostics& diagnostics, IMotion& motion);

  void init();
  void update(unsigned long nowMs);
  void onEvent(const Event& event) override;

  bool isActive() const;

private:
  void enterSafeMode(unsigned long nowMs, const char* reason);

  EventBus& eventBus_;
  Diagnostics& diagnostics_;
  IMotion& motion_;

  bool active_ = false;
};
