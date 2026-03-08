#pragma once

#include "../../core/event_bus.h"
#include "../../interfaces/i_event_listener.h"
#include "../../interfaces/i_face_controller.h"
#include "../../interfaces/i_motion.h"
#include "../../models/attention_focus.h"

class GazeService : public IEventListener {
public:
  GazeService(EventBus& eventBus, IFaceController& faceController, IMotion& motion);

  void init();
  void update(unsigned long nowMs);
  void onEvent(const Event& event) override;

private:
  void applyFocusGaze(unsigned long nowMs);

  EventBus& eventBus_;
  IFaceController& faceController_;
  IMotion& motion_;

  AttentionFocus focus_ = AttentionFocus::Idle;
  bool turnLeft_ = true;
  unsigned long lastGazeUpdateMs_ = 0;
  unsigned long lastCommandMs_ = 0;
};
