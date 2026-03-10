#pragma once

#include "../../core/diagnostics.h"
#include "../../core/event_bus.h"
#include "../../interfaces/i_action_orchestrator.h"
#include "../../interfaces/i_companion_state_provider.h"
#include "../../interfaces/i_emotion_provider.h"
#include "../../interfaces/i_event_listener.h"
#include "../../interfaces/i_face_controller.h"
#include "../../interfaces/i_motion.h"
#include "../../interfaces/i_persona_provider.h"
#include "../../interfaces/i_social_timing_provider.h"
#include "../../models/attention_focus.h"
#include "../../models/behavior_context.h"
#include "../../models/intent_types.h"
#include "../../models/mood_profile.h"
#include "../../models/persona_profile.h"
#include "../../models/routine_state.h"

class BehaviorService : public IEventListener {
public:
  BehaviorService(EventBus& eventBus,
                  const IEmotionProvider& emotionProvider,
                  const ICompanionStateProvider& companionStateProvider,
                  const IPersonaProvider& personaProvider,
                  const ISocialTimingProvider& socialTimingProvider,
                  IActionOrchestrator& actionOrchestrator,
                  IFaceController& faceController,
                  IMotion& motion,
                  Diagnostics& diagnostics);

  void init();
  void update(unsigned long nowMs);
  void onEvent(const Event& event) override;

private:
  enum class MotionCommand : uint8_t {
    None = 0,
    Center,
    IdleSway,
    CuriousLeft,
    CuriousRight,
    SoftListen,
    YawLeft,
    YawRight
  };

  enum class BehaviorActionId : uint8_t {
    None = 0,
    TouchAck,
    TiltObserve,
    ShakeRecover,
    FallRecover,
    VoiceAttend,
    EmotionCalm,
    EmotionCurious,
    EmotionPositive,
    EmotionLowEnergy,
    EmotionLowMood,
    EmotionHighArousal,
    IdleScan,
    IdleObserve,
    SocialCheckIn,
    IntentHello,
    IntentStatus,
    IntentSleep,
    IntentWake,
    IntentPhoto
  };

  struct BehaviorAction {
    BehaviorActionId id = BehaviorActionId::None;
    uint8_t priority = 0;
    ExpressionType expression = ExpressionType::Neutral;
    EyeAnimPriority facePriority = EyeAnimPriority::Idle;
    unsigned long faceHoldMs = 0;
    MotionCommand motion = MotionCommand::None;
    bool enableIdleSway = true;
  };

  BehaviorAction actionFromEvent(const Event& event);
  BehaviorAction actionFromEmotion(unsigned long nowMs);
  BehaviorAction actionFromAutonomy(unsigned long nowMs);
  BehaviorAction actionFromIntent(LocalIntent intent);
  bool tryApplyAction(const BehaviorAction& action, unsigned long nowMs);
  bool submitAction(const BehaviorAction& action, unsigned long nowMs);
  void applyMotion(MotionCommand command);
  void publishActionEvent(BehaviorActionId actionId, unsigned long nowMs);

  EventBus& eventBus_;
  const IEmotionProvider& emotionProvider_;
  const ICompanionStateProvider& companionStateProvider_;
  const IPersonaProvider& personaProvider_;
  const ISocialTimingProvider& socialTimingProvider_;
  IActionOrchestrator& actionOrchestrator_;
  IFaceController& faceController_;
  IMotion& motion_;
  Diagnostics& diagnostics_;

  BehaviorContext context_{};
  AttentionFocus preferredFocus_ = AttentionFocus::Idle;
  PersonaTone personaTone_ = PersonaTone::Warm;
  float personaInitiative_ = 0.5f;
  float personaIntensity_ = 0.5f;
  float personaSociability_ = 0.5f;
  float socialResponsiveness_ = 0.5f;
  float socialInitiative_ = 0.5f;
  float socialPersistence_ = 0.5f;
  float socialPauseFactor_ = 0.5f;
  float moodValence_ = 0.0f;
  MoodProfile moodProfile_ = MoodProfile::Calm;
  float affinityBond_ = 0.25f;
  RoutineState routineState_ = RoutineState::Idle;
  uint8_t activePriority_ = 0;
  unsigned long activeUntilMs_ = 0;
  BehaviorActionId lastActionId_ = BehaviorActionId::None;
};
