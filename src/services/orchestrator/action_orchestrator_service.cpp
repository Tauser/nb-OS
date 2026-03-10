#include "action_orchestrator_service.h"

#include "../../config/feature_flags.h"

#include <Arduino.h>

#ifndef NCOS_SIM_MODE
#define NCOS_SIM_MODE 0
#endif

ActionOrchestratorService::ActionOrchestratorService(IFaceController& faceController, IMotion& motion)
    : faceController_(faceController), motion_(motion) {}

void ActionOrchestratorService::init() {}

void ActionOrchestratorService::update(unsigned long nowMs) {
  (void)nowMs;
}

bool ActionOrchestratorService::submitIntent(const ActionIntent& intent, unsigned long nowMs) {
  if (!FeatureFlags::ACTION_ORCHESTRATOR_ENABLED) {
    return applyIntent(intent);
  }

  ChannelOwnership& ownership = ownershipFor(intent.target);
  if (nowMs >= ownership.holdUntilMs) {
    ownership.priority = 0;
    ownership.source = EventSource::Unknown;
    ownership.reason = "none";
  }

  if (nowMs < ownership.holdUntilMs && intent.priority < ownership.priority) {
    logDropped(intent, "lower_priority_active", ownership);
    return false;
  }

  if (nowMs < ownership.holdUntilMs &&
      intent.priority == ownership.priority &&
      intent.source != ownership.source) {
    logDropped(intent, "same_priority_conflict", ownership);
    return false;
  }

  const bool applied = applyIntent(intent);
  if (!applied) {
    logDropped(intent, "apply_failed", ownership);
    return false;
  }

  const char* winnerReason = "accepted";
  if (nowMs < ownership.holdUntilMs) {
    winnerReason = "override";
  }

  ownership.priority = intent.priority;
  ownership.holdUntilMs = nowMs + intent.holdMs;
  ownership.source = intent.source;
  ownership.reason = intent.reason;

  logWinner(intent, winnerReason);
  return true;
}

ActionOrchestratorService::ChannelOwnership& ActionOrchestratorService::ownershipFor(ActionTargetChannel channel) {
  switch (channel) {
    case ActionTargetChannel::Face:
      return faceOwnership_;
    case ActionTargetChannel::Motion:
      return motionOwnership_;
    case ActionTargetChannel::Led:
      return ledOwnership_;
    case ActionTargetChannel::Audio:
    default:
      return audioOwnership_;
  }
}

const ActionOrchestratorService::ChannelOwnership& ActionOrchestratorService::ownershipFor(ActionTargetChannel channel) const {
  switch (channel) {
    case ActionTargetChannel::Face:
      return faceOwnership_;
    case ActionTargetChannel::Motion:
      return motionOwnership_;
    case ActionTargetChannel::Led:
      return ledOwnership_;
    case ActionTargetChannel::Audio:
    default:
      return audioOwnership_;
  }
}

const char* ActionOrchestratorService::channelName(ActionTargetChannel channel) const {
  switch (channel) {
    case ActionTargetChannel::Face:
      return "face";
    case ActionTargetChannel::Motion:
      return "motion";
    case ActionTargetChannel::Led:
      return "led";
    case ActionTargetChannel::Audio:
      return "audio";
    default:
      return "unknown";
  }
}

const char* ActionOrchestratorService::sourceName(EventSource source) const {
  switch (source) {
    case EventSource::BehaviorService:
      return "behavior";
    case EventSource::RoutineService:
      return "routine";
    case EventSource::AttentionService:
      return "attention";
    case EventSource::GazeService:
      return "gaze";
    case EventSource::MotionSyncService:
      return "motion_sync";
    case EventSource::GestureService:
      return "gesture";
    default:
      return "unknown";
  }
}

bool ActionOrchestratorService::applyIntent(const ActionIntent& intent) {
  switch (intent.target) {
    case ActionTargetChannel::Face:
      faceController_.requestExpression(intent.face.expression, intent.face.animPriority, intent.holdMs);
      return true;
    case ActionTargetChannel::Motion:
      return applyMotionCommand(intent.motion.command);
    case ActionTargetChannel::Led:
    case ActionTargetChannel::Audio:
      // Ownership is reserved now; concrete execution will be added in future phases.
      return true;
    default:
      return false;
  }
}

bool ActionOrchestratorService::applyMotionCommand(ActionMotionCommand command) {
  switch (command) {
    case ActionMotionCommand::None:
      return true;
    case ActionMotionCommand::Center:
      motion_.center();
      return true;
    case ActionMotionCommand::IdleSway:
      motion_.idleSway();
      return true;
    case ActionMotionCommand::CuriousLeft:
      motion_.curiousLeft();
      return true;
    case ActionMotionCommand::CuriousRight:
      motion_.curiousRight();
      return true;
    case ActionMotionCommand::SoftListen:
      motion_.softListen();
      return true;
    case ActionMotionCommand::YawLeft:
      motion_.yawLeft();
      return true;
    case ActionMotionCommand::YawRight:
      motion_.yawRight();
      return true;
    case ActionMotionCommand::TiltLeft:
      motion_.tiltLeft();
      return true;
    case ActionMotionCommand::TiltRight:
      motion_.tiltRight();
      return true;
    default:
      return false;
  }
}

void ActionOrchestratorService::logWinner(const ActionIntent& intent, const char* outcomeReason) {
#if NCOS_SIM_MODE
  Serial.printf("[ACTION_ARB] channel=%s winner=%s prio=%u hold=%lu reason=%s outcome=%s\n",
                channelName(intent.target),
                sourceName(intent.source),
                static_cast<unsigned int>(intent.priority),
                intent.holdMs,
                (intent.reason != nullptr) ? intent.reason : "none",
                outcomeReason);
#else
  (void)intent;
  (void)outcomeReason;
#endif
}

void ActionOrchestratorService::logDropped(const ActionIntent& intent,
                                           const char* droppedReason,
                                           const ChannelOwnership& winner) const {
#if NCOS_SIM_MODE
  Serial.printf("[ACTION_ARB] channel=%s dropped=%s prio=%u reason=%s winner=%s winner_prio=%u winner_reason=%s\n",
                channelName(intent.target),
                sourceName(intent.source),
                static_cast<unsigned int>(intent.priority),
                droppedReason,
                sourceName(winner.source),
                static_cast<unsigned int>(winner.priority),
                (winner.reason != nullptr) ? winner.reason : "none");
#else
  (void)intent;
  (void)droppedReason;
  (void)winner;
#endif
}
