#pragma once

#include "../../core/event_bus.h"
#include "../../interfaces/i_action_orchestrator.h"
#include "../../interfaces/i_emotion_provider.h"
#include "../../interfaces/i_event_listener.h"
#include "../../interfaces/i_face_controller.h"
#include "../../interfaces/i_motion.h"
#include "../../models/power_state.h"
#include "../../models/routine_state.h"

class RoutineService : public IEventListener {
public:
  RoutineService(EventBus& eventBus,
                 const IEmotionProvider& emotionProvider,
                 IActionOrchestrator& actionOrchestrator,
                 IFaceController& faceController,
                 IMotion& motion);

  void init();
  void update(unsigned long nowMs);
  void onEvent(const Event& event) override;

private:
  enum class IdleAutonomyStage : uint8_t {
    None = 0,
    Attentive,
    Calm,
    Curious,
    Sleepy,
    Bored
  };

  void setState(RoutineState state, unsigned long nowMs);
  void publishRoutineState(unsigned long nowMs);
  void updateIdleAutonomy(unsigned long nowMs);
  IdleAutonomyStage stageForIdle(unsigned long idleForMs) const;
  void applyStage(IdleAutonomyStage stage, unsigned long nowMs, bool stageChanged);
  void markInteraction(unsigned long nowMs);
  bool submitRoutineAction(ExpressionType expression,
                           EyeAnimPriority facePriority,
                           unsigned long holdMs,
                           ActionMotionCommand motion,
                           uint8_t priority,
                           const char* reason,
                           unsigned long nowMs);

  EventBus& eventBus_;
  const IEmotionProvider& emotionProvider_;
  IActionOrchestrator& actionOrchestrator_;
  IFaceController& faceController_;
  IMotion& motion_;

  RoutineState state_ = RoutineState::Idle;
  IdleAutonomyStage idleStage_ = IdleAutonomyStage::None;
  unsigned long lastUpdateMs_ = 0;
  unsigned long lastInteractionMs_ = 0;
  unsigned long lastAutonomyStepMs_ = 0;
  unsigned long lastAttentionRecoveryMs_ = 0;
  bool nextLeft_ = true;
  PowerMode powerMode_ = PowerMode::Normal;
};
