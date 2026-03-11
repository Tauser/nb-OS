#pragma once

#include "../../core/event_bus.h"
#include "../../interfaces/i_event_listener.h"
#include "../../interfaces/i_face_controller.h"
#include "../../interfaces/i_face_render_state_provider.h"
#include "../../interfaces/i_motion.h"
#include "../../interfaces/i_motion_state_provider.h"
#include "../../models/face_sync_cue.h"

class MotionSyncService : public IEventListener {
public:
  MotionSyncService(EventBus& eventBus,
                    const IFaceRenderStateProvider& faceRenderStateProvider,
                    const IMotionStateProvider& motionStateProvider,
                    IFaceController& faceController,
                    IMotion& motion);

  void init();
  void update(unsigned long nowMs);
  void onEvent(const Event& event) override;

private:
  void syncClipWithMotion(const FaceRenderState& faceState, unsigned long nowMs);
  void syncGazeWithMotion(const FaceRenderState& faceState, unsigned long nowMs);
  void publishCue(FaceSyncCue cue, unsigned long nowMs);

  EventBus& eventBus_;
  const IFaceRenderStateProvider& faceRenderStateProvider_;
  const IMotionStateProvider& motionStateProvider_;
  IFaceController& faceController_;
  IMotion& motion_;

  bool yawLeft_ = true;
  bool lastClipActive_ = false;
  FaceClipKind lastClipKind_ = FaceClipKind::None;
  float lastGazeTargetXNorm_ = 0.0f;
  float lastCueDirectionXNorm_ = 0.0f;

  unsigned long lastSyncMs_ = 0;
  unsigned long lastGazeSyncMs_ = 0;
  unsigned long lastClipSyncMs_ = 0;
  unsigned long lastCueMs_ = 0;
  unsigned long lastCenterRecoveryMs_ = 0;
};
