#pragma once

#include "../../core/event_bus.h"
#include "../../interfaces/i_event_listener.h"
#include "../../interfaces/i_face_controller.h"
#include "../../interfaces/i_motion.h"
#include "../../models/gesture_types.h"

class GestureService : public IEventListener {
public:
  GestureService(EventBus& eventBus, IFaceController& faceController, IMotion& motion);

  void init();
  void update(unsigned long nowMs);
  void onEvent(const Event& event) override;

private:
  bool playGesture(GestureType gesture, unsigned long nowMs);
  void publishGesture(GestureType gesture, unsigned long nowMs);

  EventBus& eventBus_;
  IFaceController& faceController_;
  IMotion& motion_;

  unsigned long lastGestureMs_ = 0;
};
