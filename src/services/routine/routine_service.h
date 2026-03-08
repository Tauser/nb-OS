#pragma once

#include "../../core/event_bus.h"
#include "../../interfaces/i_event_listener.h"
#include "../../interfaces/i_face_controller.h"
#include "../../interfaces/i_motion.h"
#include "../../models/routine_state.h"

class RoutineService : public IEventListener {
public:
  RoutineService(EventBus& eventBus, IFaceController& faceController, IMotion& motion);

  void init();
  void update(unsigned long nowMs);
  void onEvent(const Event& event) override;

private:
  void setState(RoutineState state, unsigned long nowMs);
  void publishRoutineState(unsigned long nowMs);

  EventBus& eventBus_;
  IFaceController& faceController_;
  IMotion& motion_;

  RoutineState state_ = RoutineState::Idle;
  unsigned long lastUpdateMs_ = 0;
  unsigned long lastInteractionMs_ = 0;
};
