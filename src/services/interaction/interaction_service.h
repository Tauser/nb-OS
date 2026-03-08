#pragma once

#include "../../core/diagnostics.h"
#include "../../core/event_bus.h"
#include "../../interfaces/i_event_listener.h"

class InteractionService : public IEventListener {
public:
  InteractionService(EventBus& eventBus, Diagnostics& diagnostics);

  void init();
  void update();
  void onEvent(const Event& event) override;

private:
  EventBus& eventBus_;
  Diagnostics& diagnostics_;

  unsigned long touchCount_ = 0;
  unsigned long shakeCount_ = 0;
  unsigned long tiltCount_ = 0;
  unsigned long fallCount_ = 0;
  unsigned long motionPoseCount_ = 0;
};
