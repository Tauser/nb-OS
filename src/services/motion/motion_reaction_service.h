#pragma once

#include "../../core/event_bus.h"
#include "../../interfaces/i_event_listener.h"
#include "../../interfaces/i_motion.h"

class MotionReactionService : public IEventListener {
public:
  MotionReactionService(EventBus& eventBus, IMotion& motion);

  void init();
  void update(unsigned long nowMs);
  void onEvent(const Event& event) override;

private:
  EventBus& eventBus_;
  IMotion& motion_;

  bool curiousLeftNext_ = true;
  unsigned long lastReactionMs_ = 0;
};
