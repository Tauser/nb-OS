#pragma once

#include "../../interfaces/i_action_orchestrator.h"
#include "../../interfaces/i_face_controller.h"
#include "../../interfaces/i_motion.h"

class ActionOrchestratorService : public IActionOrchestrator {
public:
  ActionOrchestratorService(IFaceController& faceController, IMotion& motion);

  void init() override;
  void update(unsigned long nowMs) override;
  bool submitIntent(const ActionIntent& intent, unsigned long nowMs) override;

private:
  struct ChannelOwnership {
    uint8_t priority = 0;
    unsigned long holdUntilMs = 0;
    EventSource source = EventSource::Unknown;
    const char* reason = "none";
  };

  ChannelOwnership& ownershipFor(ActionTargetChannel channel);
  const ChannelOwnership& ownershipFor(ActionTargetChannel channel) const;
  const char* channelName(ActionTargetChannel channel) const;
  const char* sourceName(EventSource source) const;
  bool applyIntent(const ActionIntent& intent);
  bool applyMotionCommand(ActionMotionCommand command);
  void logWinner(const ActionIntent& intent, const char* outcomeReason);
  void logDropped(const ActionIntent& intent,
                  const char* droppedReason,
                  const ChannelOwnership& winner) const;

  IFaceController& faceController_;
  IMotion& motion_;

  ChannelOwnership faceOwnership_{};
  ChannelOwnership motionOwnership_{};
  ChannelOwnership ledOwnership_{};
  ChannelOwnership audioOwnership_{};
};
