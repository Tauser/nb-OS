#pragma once

#include "../../core/event_bus.h"
#include "../../interfaces/i_event_listener.h"
#include "../../interfaces/i_face_controller.h"
#include "../../interfaces/i_motion.h"

class MotionSyncService : public IEventListener {
public:
  MotionSyncService(EventBus& eventBus, IFaceController& faceController, IMotion& motion);

  void init();
  void update(unsigned long nowMs);
  void onEvent(const Event& event) override;

private:
  EventBus& eventBus_;
  IFaceController& faceController_;
  IMotion& motion_;

  bool yawLeft_ = true;
  unsigned long lastSyncMs_ = 0;
};
