#include "behavior_service.h"

#include "../../config/hardware_config.h"
#include "../../models/event.h"
#include "../../utils/math_utils.h"
#include <Arduino.h>

#ifndef NCOS_SIM_MODE
#define NCOS_SIM_MODE 0
#endif

namespace {
const char* kBehaviorEventLog = "[BEHAVIOR] action";
}

BehaviorService::BehaviorService(EventBus& eventBus,
                                 const IEmotionProvider& emotionProvider,
                                 const IPersonaProvider& personaProvider,
                                 const ISocialTimingProvider& socialTimingProvider,
                                 IFaceController& faceController,
                                 IMotion& motion,
                                 Diagnostics& diagnostics)
    : eventBus_(eventBus),
      emotionProvider_(emotionProvider),
      personaProvider_(personaProvider),
      socialTimingProvider_(socialTimingProvider),
      faceController_(faceController),
      motion_(motion),
      diagnostics_(diagnostics) {}

void BehaviorService::init() {
  eventBus_.subscribe(EventType::EVT_TOUCH, this);
  eventBus_.subscribe(EventType::EVT_SHAKE, this);
  eventBus_.subscribe(EventType::EVT_TILT, this);
  eventBus_.subscribe(EventType::EVT_FALL, this);
  eventBus_.subscribe(EventType::EVT_IDLE, this);
  eventBus_.subscribe(EventType::EVT_VOICE_START, this);
  eventBus_.subscribe(EventType::EVT_VOICE_ACTIVITY, this);
  eventBus_.subscribe(EventType::EVT_VOICE_END, this);
  eventBus_.subscribe(EventType::EVT_EMOTION_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_INTENT_DETECTED, this);
  eventBus_.subscribe(EventType::EVT_MOOD_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_MOOD_PROFILE_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_AFFINITY_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_PREFERENCE_UPDATED, this);
  eventBus_.subscribe(EventType::EVT_PERSONA_UPDATED, this);
  eventBus_.subscribe(EventType::EVT_ROUTINE_STATE_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_POWER_STATUS, this);
  eventBus_.subscribe(EventType::EVT_CHARGING_STATE_CHANGED, this);

  const unsigned long nowMs = millis();
  context_.lastInteractionMs = nowMs;
  context_.lastIdleEvalMs = nowMs;
}

void BehaviorService::update(unsigned long nowMs) {
  if (activePriority_ > 0 && nowMs >= activeUntilMs_) {
    activePriority_ = 0;
  }

  if (nowMs - context_.lastIdleEvalMs < HardwareConfig::Behavior::IDLE_EVAL_INTERVAL_MS) {
    return;
  }

  context_.lastIdleEvalMs = nowMs;

  const PersonaProfile& persona = personaProvider_.getProfile();
  personaTone_ = persona.tone;
  personaInitiative_ = persona.initiative;
  personaIntensity_ = persona.intensity;
  personaSociability_ = persona.sociability;

  const SocialTimingState& socialTiming = socialTimingProvider_.getState();
  socialResponsiveness_ = socialTiming.responsiveness;
  socialInitiative_ = socialTiming.initiative;
  socialPersistence_ = socialTiming.persistence;
  socialPauseFactor_ = socialTiming.pauseFactor;

  bool applied = tryApplyAction(actionFromEmotion(nowMs), nowMs);
  if (!applied) {
    tryApplyAction(actionFromAutonomy(nowMs), nowMs);
  }
}

void BehaviorService::onEvent(const Event& event) {
  if (event.type == EventType::EVT_BEHAVIOR_ACTION || event.source == EventSource::BehaviorService) {
    return;
  }

  if (event.type == EventType::EVT_MOOD_CHANGED) {
    moodValence_ = static_cast<float>(event.value) / 1000.0f;
    return;
  }

  if (event.type == EventType::EVT_MOOD_PROFILE_CHANGED) {
    moodProfile_ = static_cast<MoodProfile>(event.value);
    return;
  }

  if (event.type == EventType::EVT_AFFINITY_CHANGED) {
    affinityBond_ = static_cast<float>(event.value) / 1000.0f;
    return;
  }

  if (event.type == EventType::EVT_PREFERENCE_UPDATED) {
    preferredFocus_ = static_cast<AttentionFocus>(event.value);
    return;
  }

  if (event.type == EventType::EVT_PERSONA_UPDATED) {
    personaTone_ = static_cast<PersonaTone>(event.value);
    return;
  }

  if (event.type == EventType::EVT_ROUTINE_STATE_CHANGED) {
    routineState_ = static_cast<RoutineState>(event.value);
    return;
  }

  const unsigned long nowMs = event.timestamp;
  context_.lastEventType = event.type;
  context_.lastEventMs = nowMs;

  if (event.type == EventType::EVT_TOUCH ||
      event.type == EventType::EVT_TILT ||
      event.type == EventType::EVT_SHAKE ||
      event.type == EventType::EVT_FALL ||
      event.type == EventType::EVT_VOICE_START ||
      event.type == EventType::EVT_VOICE_ACTIVITY ||
      event.type == EventType::EVT_INTENT_DETECTED) {
    context_.lastInteractionMs = nowMs;
    context_.idleTicks = 0;
  }

  if (event.type == EventType::EVT_IDLE) {
    context_.idleTicks++;
    return;
  }

  if (event.type == EventType::EVT_INTENT_DETECTED) {
    const LocalIntent intent = static_cast<LocalIntent>(event.value);
    tryApplyAction(actionFromIntent(intent), nowMs);
    return;
  }

  tryApplyAction(actionFromEvent(event), nowMs);
}
BehaviorService::BehaviorAction BehaviorService::actionFromEvent(const Event& event) {
  BehaviorAction action;

  switch (event.type) {
    case EventType::EVT_TOUCH:
      action.id = BehaviorActionId::TouchAck;
      action.priority = 70;
      action.expression = ExpressionType::FaceRecognized;
      action.facePriority = EyeAnimPriority::Social;
      action.faceHoldMs = HardwareConfig::Behavior::TOUCH_FACE_HOLD_MS;
      action.motion = context_.nextCuriousLeft ? MotionCommand::CuriousLeft : MotionCommand::CuriousRight;
      action.enableIdleSway = true;
      break;

    case EventType::EVT_TILT:
      action.id = BehaviorActionId::TiltObserve;
      action.priority = 60;
      action.expression = (moodProfile_ == MoodProfile::Reserved || moodProfile_ == MoodProfile::Sensitive) ? ExpressionType::Shy : ExpressionType::Curiosity;
      action.facePriority = EyeAnimPriority::Social;
      action.faceHoldMs = HardwareConfig::Behavior::TILT_FACE_HOLD_MS;
      action.motion = MotionCommand::SoftListen;
      action.enableIdleSway = true;
      break;

    case EventType::EVT_SHAKE:
      action.id = BehaviorActionId::ShakeRecover;
      action.priority = 85;
      action.expression = ExpressionType::Surprised;
      action.facePriority = EyeAnimPriority::Alert;
      action.faceHoldMs = HardwareConfig::Behavior::SHAKE_FACE_HOLD_MS;
      action.motion = MotionCommand::Center;
      action.enableIdleSway = true;
      break;

    case EventType::EVT_FALL:
      action.id = BehaviorActionId::FallRecover;
      action.priority = 100;
      action.expression = ExpressionType::Surprised;
      action.facePriority = EyeAnimPriority::Critical;
      action.faceHoldMs = HardwareConfig::Behavior::FALL_FACE_HOLD_MS;
      action.motion = MotionCommand::Center;
      action.enableIdleSway = true;
      break;

    case EventType::EVT_VOICE_START:
    case EventType::EVT_VOICE_ACTIVITY:
      action.id = BehaviorActionId::VoiceAttend;
      action.priority = 65;
      action.expression = ExpressionType::Listening;
      action.facePriority = EyeAnimPriority::Social;
      action.faceHoldMs = HardwareConfig::Behavior::TILT_FACE_HOLD_MS;
      action.motion = MotionCommand::SoftListen;
      action.enableIdleSway = true;
      break;

    default:
      break;
  }

  return action;
}

BehaviorService::BehaviorAction BehaviorService::actionFromEmotion(unsigned long nowMs) {
  if (routineState_ == RoutineState::Charging ||
      routineState_ == RoutineState::Rest ||
      routineState_ == RoutineState::Listening ||
      routineState_ == RoutineState::Sleepy) {
    return BehaviorAction{};
  }

  BehaviorAction action;
  const unsigned long personaHoldBoost = static_cast<unsigned long>(personaIntensity_ * 140.0f);
  const uint8_t personaPriorityBoost = static_cast<uint8_t>(personaInitiative_ * 4.0f);

  switch (moodProfile_) {
    case MoodProfile::Sleepy:
      action.id = BehaviorActionId::EmotionLowEnergy;
      action.priority = 37;
      action.expression = ExpressionType::BatteryAlert;
      action.facePriority = EyeAnimPriority::Emotion;
      action.faceHoldMs = HardwareConfig::Behavior::EMOTION_FACE_HOLD_MS + 220;
      action.motion = MotionCommand::SoftListen;
      action.enableIdleSway = true;
      return action;

    case MoodProfile::Curious:
      action.id = BehaviorActionId::EmotionCurious;
      action.priority = static_cast<uint8_t>(39 + personaPriorityBoost);
      action.expression = ExpressionType::Curiosity;
      action.facePriority = EyeAnimPriority::Emotion;
      action.faceHoldMs = HardwareConfig::Behavior::EMOTION_FACE_HOLD_MS + 120 + personaHoldBoost;
      action.motion = context_.nextCuriousLeft ? MotionCommand::CuriousLeft : MotionCommand::CuriousRight;
      action.enableIdleSway = true;
      return action;

    case MoodProfile::Sensitive:
      action.id = BehaviorActionId::EmotionLowMood;
      action.priority = 36;
      action.expression = ExpressionType::Sad;
      action.facePriority = EyeAnimPriority::Emotion;
      action.faceHoldMs = HardwareConfig::Behavior::EMOTION_FACE_HOLD_MS + 160;
      action.motion = MotionCommand::Center;
      action.enableIdleSway = true;
      return action;

    case MoodProfile::Social:
      action.id = BehaviorActionId::EmotionPositive;
      action.priority = static_cast<uint8_t>(38 + personaPriorityBoost);
      action.expression = ExpressionType::FaceRecognized;
      action.facePriority = EyeAnimPriority::Emotion;
      action.faceHoldMs = HardwareConfig::Behavior::EMOTION_FACE_HOLD_MS + 120 + personaHoldBoost;
      action.motion = MotionCommand::IdleSway;
      action.enableIdleSway = true;
      return action;

    case MoodProfile::Reserved:
      action.id = BehaviorActionId::EmotionCalm;
      action.priority = 31;
      action.expression = ExpressionType::Neutral;
      action.facePriority = EyeAnimPriority::Idle;
      action.faceHoldMs = HardwareConfig::Behavior::EMOTION_FACE_HOLD_MS + 100 + (personaHoldBoost / 2);
      action.motion = MotionCommand::SoftListen;
      action.enableIdleSway = true;
      return action;

    case MoodProfile::Animated:
      action.id = BehaviorActionId::EmotionHighArousal;
      action.priority = 35;
      action.expression = ExpressionType::FaceRecognized;
      action.facePriority = EyeAnimPriority::Emotion;
      action.faceHoldMs = HardwareConfig::Behavior::EMOTION_FACE_HOLD_MS;
      action.motion = context_.nextYawLeft ? MotionCommand::YawLeft : MotionCommand::YawRight;
      action.enableIdleSway = true;
      return action;

    case MoodProfile::Calm:
    default:
      break;
  }

  const EmotionState& emo = emotionProvider_.getEmotionState();

  if (emo.energy <= HardwareConfig::Behavior::EMOTION_LOW_ENERGY) {
    action.id = BehaviorActionId::EmotionLowEnergy;
    action.priority = 30;
    action.expression = ExpressionType::Sad;
    action.facePriority = EyeAnimPriority::Emotion;
    action.faceHoldMs = HardwareConfig::Behavior::EMOTION_FACE_HOLD_MS;
    action.motion = MotionCommand::SoftListen;
    action.enableIdleSway = true;
    return action;
  }

  if (emo.valence <= HardwareConfig::Behavior::EMOTION_NEG_VALENCE || moodValence_ < -0.35f) {
    action.id = BehaviorActionId::EmotionLowMood;
    action.priority = 32;
    action.expression = ExpressionType::Sad;
    action.facePriority = EyeAnimPriority::Emotion;
    action.faceHoldMs = HardwareConfig::Behavior::EMOTION_FACE_HOLD_MS;
    action.motion = MotionCommand::Center;
    action.enableIdleSway = true;
    return action;
  }

  if (emo.curiosity >= HardwareConfig::Behavior::EMOTION_HIGH_CURIOSITY ||
      (nowMs - context_.lastInteractionMs >= HardwareConfig::Behavior::LONG_IDLE_MS)) {
    action.id = BehaviorActionId::EmotionCurious;
    action.priority = 34;
    action.expression = (personaTone_ == PersonaTone::Calm) ? ExpressionType::Neutral : ExpressionType::Curiosity;
    action.facePriority = EyeAnimPriority::Emotion;
    action.faceHoldMs = HardwareConfig::Behavior::EMOTION_FACE_HOLD_MS;
    action.motion = context_.nextCuriousLeft ? MotionCommand::CuriousLeft : MotionCommand::CuriousRight;
    action.enableIdleSway = true;
    return action;
  }

  action.id = BehaviorActionId::EmotionCalm;
  action.priority = 20;
  action.expression = ExpressionType::Neutral;
  action.facePriority = EyeAnimPriority::Idle;
  action.faceHoldMs = HardwareConfig::Behavior::EMOTION_FACE_HOLD_MS;
  action.motion = MotionCommand::IdleSway;
  action.enableIdleSway = true;
  return action;
}

BehaviorService::BehaviorAction BehaviorService::actionFromAutonomy(unsigned long nowMs) {
  BehaviorAction action;
  const EmotionState& emo = emotionProvider_.getEmotionState();
  const unsigned long idleForMs = nowMs - context_.lastInteractionMs;
  const uint8_t personaPriorityBoost = static_cast<uint8_t>(personaInitiative_ * 3.0f);
  const unsigned long personaHoldBoost = static_cast<unsigned long>(personaIntensity_ * 120.0f);

  const float cooldownScale = MathUtils::clamp(
      1.30f - (socialResponsiveness_ * 0.55f) + (socialPauseFactor_ * 0.45f),
      0.55f,
      1.80f);
  const unsigned long dynamicCooldownMs = static_cast<unsigned long>(
      static_cast<float>(HardwareConfig::Behavior::AUTONOMY_ACTION_COOLDOWN_MS) * cooldownScale);

  const float socialIntervalScale = MathUtils::clamp(
      1.35f - (socialInitiative_ * 0.55f) - (socialPersistence_ * 0.25f) + (socialPauseFactor_ * 0.35f),
      0.45f,
      1.90f);
  const unsigned long socialIntervalMs = static_cast<unsigned long>(
      static_cast<float>(HardwareConfig::Behavior::SOCIAL_INITIATIVE_INTERVAL_MS) * socialIntervalScale);

  if (idleForMs < HardwareConfig::Behavior::AUTONOMY_MIN_IDLE_MS) {
    return action;
  }

  if (nowMs - context_.lastAutonomyActionMs < dynamicCooldownMs) {
    return action;
  }

  if (routineState_ == RoutineState::Listening || routineState_ == RoutineState::Charging || routineState_ == RoutineState::Sleepy) {
    return action;
  }

  if (moodProfile_ == MoodProfile::Reserved && idleForMs < HardwareConfig::Behavior::AUTONOMY_BORED_IDLE_MS) {
    action.id = BehaviorActionId::IdleObserve;
    action.priority = 37;
    action.expression = ExpressionType::Neutral;
    action.facePriority = EyeAnimPriority::Idle;
    action.faceHoldMs = HardwareConfig::Behavior::EMOTION_FACE_HOLD_MS + 220;
    action.motion = MotionCommand::SoftListen;
    action.enableIdleSway = true;
    return action;
  }

  const float socialBondMin = MathUtils::clamp(
      HardwareConfig::Behavior::SOCIAL_INITIATIVE_BOND_MIN -
          (personaSociability_ * 0.12f) - (socialInitiative_ * 0.10f),
      0.14f,
      0.60f);

  const float socialAttentionMin = MathUtils::clamp(
      HardwareConfig::Behavior::SOCIAL_INITIATIVE_ATTENTION_MIN - (socialResponsiveness_ * 0.20f),
      0.28f,
      0.80f);

  if (moodProfile_ == MoodProfile::Social &&
      affinityBond_ >= socialBondMin) {
    action.id = BehaviorActionId::SocialCheckIn;
    action.priority = 48;
    action.expression = ExpressionType::FaceRecognized;
    action.facePriority = EyeAnimPriority::Social;
    action.faceHoldMs = HardwareConfig::Behavior::TOUCH_FACE_HOLD_MS + 120 + personaHoldBoost;
    action.motion = (preferredFocus_ == AttentionFocus::Voice)
                        ? MotionCommand::SoftListen
                        : ((context_.autonomyTicks % 2 == 0) ? MotionCommand::CuriousLeft : MotionCommand::CuriousRight);
    action.enableIdleSway = true;
    return action;
  }

  if (routineState_ == RoutineState::Sleepy || routineState_ == RoutineState::Rest ||
      idleForMs >= HardwareConfig::Behavior::AUTONOMY_BORED_IDLE_MS) {
    action.id = BehaviorActionId::IdleObserve;
    action.priority = static_cast<uint8_t>(38 + personaPriorityBoost);
    action.expression = (routineState_ == RoutineState::Bored) ? ExpressionType::Sad : ExpressionType::BatteryAlert;
    action.facePriority = EyeAnimPriority::Idle;
    action.faceHoldMs = HardwareConfig::Behavior::EMOTION_FACE_HOLD_MS + 220;
    action.motion = MotionCommand::SoftListen;
    action.enableIdleSway = true;
    return action;
  }

  if (emo.attention >= socialAttentionMin &&
      affinityBond_ >= socialBondMin &&
      nowMs - context_.lastSocialInitiativeMs >= socialIntervalMs) {
    action.id = BehaviorActionId::SocialCheckIn;
    action.priority = static_cast<uint8_t>(46 + personaPriorityBoost);
    action.expression = ExpressionType::FaceRecognized;
    action.facePriority = EyeAnimPriority::Social;
    action.faceHoldMs = HardwareConfig::Behavior::TOUCH_FACE_HOLD_MS;
    action.motion = (preferredFocus_ == AttentionFocus::Voice)
                        ? MotionCommand::SoftListen
                        : ((context_.autonomyTicks % 2 == 0) ? MotionCommand::CuriousLeft : MotionCommand::CuriousRight);
    action.enableIdleSway = true;
    return action;
  }

  if (emo.curiosity >= HardwareConfig::Behavior::AUTONOMY_CURIOSITY_SCAN_MIN &&
      context_.idleTicks >= HardwareConfig::Behavior::AUTONOMY_IDLE_TICKS_FOR_SCAN) {
    action.id = BehaviorActionId::IdleScan;
    action.priority = 36;
    action.expression = ExpressionType::Curiosity;
    action.facePriority = EyeAnimPriority::Idle;
    action.faceHoldMs = HardwareConfig::Behavior::EMOTION_FACE_HOLD_MS;
    action.motion = context_.nextCuriousLeft ? MotionCommand::CuriousLeft : MotionCommand::CuriousRight;
    action.enableIdleSway = true;
    return action;
  }

  if (idleForMs >= HardwareConfig::Behavior::LONG_IDLE_MS) {
    action.id = BehaviorActionId::IdleObserve;
    action.priority = 34;
    action.expression = ExpressionType::Neutral;
    action.facePriority = EyeAnimPriority::Idle;
    action.faceHoldMs = HardwareConfig::Behavior::EMOTION_FACE_HOLD_MS;
    action.motion = context_.nextYawLeft ? MotionCommand::YawLeft : MotionCommand::YawRight;
    action.enableIdleSway = true;
    return action;
  }

  return action;
}
BehaviorService::BehaviorAction BehaviorService::actionFromIntent(LocalIntent intent) {
  BehaviorAction action;

  switch (intent) {
    case LocalIntent::Hello:
      action.id = BehaviorActionId::IntentHello;
      action.priority = 72;
      action.expression = ExpressionType::FaceRecognized;
      action.facePriority = EyeAnimPriority::Social;
      action.faceHoldMs = HardwareConfig::Behavior::TOUCH_FACE_HOLD_MS;
      action.motion = MotionCommand::CuriousRight;
      action.enableIdleSway = true;
      break;

    case LocalIntent::Status:
      action.id = BehaviorActionId::IntentStatus;
      action.priority = 78;
      action.expression = ExpressionType::Thinking;
      action.facePriority = EyeAnimPriority::Social;
      action.faceHoldMs = HardwareConfig::Behavior::TILT_FACE_HOLD_MS + 220;
      action.motion = MotionCommand::SoftListen;
      action.enableIdleSway = true;
      break;

    case LocalIntent::Sleep:
      action.id = BehaviorActionId::IntentSleep;
      action.priority = 82;
      action.expression = ExpressionType::Sad;
      action.facePriority = EyeAnimPriority::Alert;
      action.faceHoldMs = HardwareConfig::Behavior::SHAKE_FACE_HOLD_MS + 300;
      action.motion = MotionCommand::SoftListen;
      action.enableIdleSway = false;
      break;

    case LocalIntent::Wake:
      action.id = BehaviorActionId::IntentWake;
      action.priority = 84;
      action.expression = ExpressionType::FaceRecognized;
      action.facePriority = EyeAnimPriority::Alert;
      action.faceHoldMs = HardwareConfig::Behavior::SHAKE_FACE_HOLD_MS + 260;
      action.motion = MotionCommand::Center;
      action.enableIdleSway = true;
      break;

    case LocalIntent::Photo:
      action.id = BehaviorActionId::IntentPhoto;
      action.priority = 90;
      action.expression = ExpressionType::Curiosity;
      action.facePriority = EyeAnimPriority::Critical;
      action.faceHoldMs = HardwareConfig::Behavior::FALL_FACE_HOLD_MS;
      action.motion = MotionCommand::Center;
      action.enableIdleSway = false;
      break;

    default:
      break;
  }

  return action;
}

bool BehaviorService::tryApplyAction(const BehaviorAction& action, unsigned long nowMs) {
  if (action.id == BehaviorActionId::None) {
    return false;
  }

  if (nowMs - context_.lastActionMs < HardwareConfig::Behavior::ACTION_MIN_INTERVAL_MS &&
      action.priority <= activePriority_) {
    return false;
  }

  if (action.id == lastActionId_ &&
      nowMs - context_.lastActionMs < HardwareConfig::Behavior::REPEAT_SUPPRESS_MS) {
    return false;
  }

  if (nowMs < activeUntilMs_ && action.priority < activePriority_) {
    return false;
  }

  if (action.motion == MotionCommand::CuriousLeft || action.motion == MotionCommand::CuriousRight) {
    context_.nextCuriousLeft = !context_.nextCuriousLeft;
  } else if (action.motion == MotionCommand::YawLeft || action.motion == MotionCommand::YawRight) {
    context_.nextYawLeft = !context_.nextYawLeft;
  }

  faceController_.requestExpression(action.expression, action.facePriority, action.faceHoldMs);
  applyMotion(action.motion);
  if (action.enableIdleSway) {
    motion_.idleSway();
  }

  if (action.id == BehaviorActionId::IdleScan ||
      action.id == BehaviorActionId::IdleObserve ||
      action.id == BehaviorActionId::SocialCheckIn) {
    context_.autonomyTicks++;
    context_.lastAutonomyActionMs = nowMs;

    if (action.id == BehaviorActionId::SocialCheckIn) {
      context_.lastSocialInitiativeMs = nowMs;
    }
  }

  activePriority_ = action.priority;
  activeUntilMs_ = nowMs + action.faceHoldMs;
  context_.lastActionMs = nowMs;
  lastActionId_ = action.id;

  publishActionEvent(action.id, nowMs);
  #if !NCOS_SIM_MODE
  diagnostics_.logInfo(kBehaviorEventLog);
  #endif
  return true;
}

void BehaviorService::applyMotion(MotionCommand command) {
  switch (command) {
    case MotionCommand::Center:
      motion_.center();
      break;
    case MotionCommand::IdleSway:
      motion_.idleSway();
      break;
    case MotionCommand::CuriousLeft:
      motion_.curiousLeft();
      break;
    case MotionCommand::CuriousRight:
      motion_.curiousRight();
      break;
    case MotionCommand::SoftListen:
      motion_.softListen();
      break;
    case MotionCommand::YawLeft:
      motion_.yawLeft();
      break;
    case MotionCommand::YawRight:
      motion_.yawRight();
      break;
    default:
      break;
  }
}

void BehaviorService::publishActionEvent(BehaviorActionId actionId, unsigned long nowMs) {
  Event event;
  event.type = EventType::EVT_BEHAVIOR_ACTION;
  event.source = EventSource::BehaviorService;
  event.value = static_cast<int>(actionId);
  event.timestamp = nowMs;
  eventBus_.publish(event);
}




















