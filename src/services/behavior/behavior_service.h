#pragma once

#include "../../core/diagnostics.h"
#include "../../core/event_bus.h"
#include "../../interfaces/i_emotion_provider.h"
#include "../../interfaces/i_event_listener.h"
#include "../../interfaces/i_face_controller.h"
#include "../../interfaces/i_motion.h"
#include "../../models/attention_focus.h"
#include "../../models/behavior_context.h"
#include "../../models/intent_types.h"
#include "../../models/persona_profile.h"

class BehaviorService : public IEventListener {
public:
  BehaviorService(EventBus& eventBus,
                  const IEmotionProvider& emotionProvider,
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
  void applyMotion(MotionCommand command);
  void publishActionEvent(BehaviorActionId actionId, unsigned long nowMs);

  EventBus& eventBus_;
  const IEmotionProvider& emotionProvider_;
  IFaceController& faceController_;
  IMotion& motion_;
  Diagnostics& diagnostics_;

  BehaviorContext context_{};
  AttentionFocus preferredFocus_ = AttentionFocus::Idle;
  PersonaTone personaTone_ = PersonaTone::Warm;
  float moodValence_ = 0.0f;
  float affinityBond_ = 0.25f;
  uint8_t activePriority_ = 0;
  unsigned long activeUntilMs_ = 0;
};
