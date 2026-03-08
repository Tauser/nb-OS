#include "behavior_service.h"

#include "../../config/hardware_config.h"
#include "../../models/event.h"
#include <Arduino.h>

namespace {
const char* kBehaviorEventLog = "[BEHAVIOR] action";
}

BehaviorService::BehaviorService(EventBus& eventBus,
                                 const IEmotionProvider& emotionProvider,
                                 IFaceController& faceController,
                                 IMotion& motion,
                                 Diagnostics& diagnostics)
    : eventBus_(eventBus),
      emotionProvider_(emotionProvider),
      faceController_(faceController),
      motion_(motion),
      diagnostics_(diagnostics) {}

void BehaviorService::init() {
  eventBus_.subscribe(EventType::EVT_TOUCH, this);
  eventBus_.subscribe(EventType::EVT_SHAKE, this);
  eventBus_.subscribe(EventType::EVT_TILT, this);
  eventBus_.subscribe(EventType::EVT_FALL, this);
  eventBus_.subscribe(EventType::EVT_IDLE, this);
  eventBus_.subscribe(EventType::EVT_VOICE_ACTIVITY, this);
  eventBus_.subscribe(EventType::EVT_EMOTION_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_INTENT_DETECTED, this);

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

  bool applied = tryApplyAction(actionFromEmotion(nowMs), nowMs);
  if (!applied) {
    tryApplyAction(actionFromAutonomy(nowMs), nowMs);
  }
}

void BehaviorService::onEvent(const Event& event) {
  if (event.type == EventType::EVT_BEHAVIOR_ACTION || event.source == EventSource::BehaviorService) {
    return;
  }

  const unsigned long nowMs = event.timestamp;
  context_.lastEventType = event.type;
  context_.lastEventMs = nowMs;

  if (event.type == EventType::EVT_TOUCH ||
      event.type == EventType::EVT_TILT ||
      event.type == EventType::EVT_SHAKE ||
      event.type == EventType::EVT_FALL ||
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
      action.expression = ExpressionType::Curiosity;
      action.facePriority = EyeAnimPriority::Social;
      action.faceHoldMs = HardwareConfig::Behavior::TILT_FACE_HOLD_MS;
      action.motion = MotionCommand::SoftListen;
      action.enableIdleSway = true;
      break;

    case EventType::EVT_SHAKE:
      action.id = BehaviorActionId::ShakeRecover;
      action.priority = 85;
      action.expression = ExpressionType::Angry;
      action.facePriority = EyeAnimPriority::Alert;
      action.faceHoldMs = HardwareConfig::Behavior::SHAKE_FACE_HOLD_MS;
      action.motion = MotionCommand::Center;
      action.enableIdleSway = true;
      break;

    case EventType::EVT_FALL:
      action.id = BehaviorActionId::FallRecover;
      action.priority = 100;
      action.expression = ExpressionType::BatteryAlert;
      action.facePriority = EyeAnimPriority::Critical;
      action.faceHoldMs = HardwareConfig::Behavior::FALL_FACE_HOLD_MS;
      action.motion = MotionCommand::Center;
      action.enableIdleSway = true;
      break;

    case EventType::EVT_VOICE_ACTIVITY:
      action.id = BehaviorActionId::VoiceAttend;
      action.priority = 65;
      action.expression = ExpressionType::FaceRecognized;
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
  const EmotionState& emo = emotionProvider_.getEmotionState();
  BehaviorAction action;

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

  if (emo.valence <= HardwareConfig::Behavior::EMOTION_NEG_VALENCE) {
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
    action.priority = 40;
    action.expression = ExpressionType::Curiosity;
    action.facePriority = EyeAnimPriority::Emotion;
    action.faceHoldMs = HardwareConfig::Behavior::EMOTION_FACE_HOLD_MS;
    action.motion = context_.nextCuriousLeft ? MotionCommand::CuriousLeft : MotionCommand::CuriousRight;
    action.enableIdleSway = true;
    return action;
  }

  if (emo.arousal >= HardwareConfig::Behavior::EMOTION_HIGH_AROUSAL) {
    action.id = BehaviorActionId::EmotionHighArousal;
    action.priority = 34;
    action.expression = ExpressionType::FaceRecognized;
    action.facePriority = EyeAnimPriority::Emotion;
    action.faceHoldMs = HardwareConfig::Behavior::EMOTION_FACE_HOLD_MS;
    action.motion = context_.nextYawLeft ? MotionCommand::YawLeft : MotionCommand::YawRight;
    action.enableIdleSway = true;
    return action;
  }

  if (emo.valence >= HardwareConfig::Behavior::EMOTION_POS_VALENCE) {
    action.id = BehaviorActionId::EmotionPositive;
    action.priority = 33;
    action.expression = ExpressionType::FaceRecognized;
    action.facePriority = EyeAnimPriority::Emotion;
    action.faceHoldMs = HardwareConfig::Behavior::EMOTION_FACE_HOLD_MS;
    action.motion = MotionCommand::IdleSway;
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

  if (idleForMs < HardwareConfig::Behavior::AUTONOMY_MIN_IDLE_MS) {
    return action;
  }

  if (nowMs - context_.lastAutonomyActionMs < HardwareConfig::Behavior::AUTONOMY_ACTION_COOLDOWN_MS) {
    return action;
  }

  if (emo.attention >= HardwareConfig::Behavior::SOCIAL_INITIATIVE_ATTENTION_MIN &&
      emo.bond >= HardwareConfig::Behavior::SOCIAL_INITIATIVE_BOND_MIN &&
      nowMs - context_.lastSocialInitiativeMs >= HardwareConfig::Behavior::SOCIAL_INITIATIVE_INTERVAL_MS) {
    action.id = BehaviorActionId::SocialCheckIn;
    action.priority = 46;
    action.expression = ExpressionType::FaceRecognized;
    action.facePriority = EyeAnimPriority::Social;
    action.faceHoldMs = HardwareConfig::Behavior::TOUCH_FACE_HOLD_MS;
    action.motion = (context_.autonomyTicks % 2 == 0) ? MotionCommand::SoftListen : MotionCommand::CuriousRight;
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
      action.expression = ExpressionType::Curiosity;
      action.facePriority = EyeAnimPriority::Social;
      action.faceHoldMs = HardwareConfig::Behavior::TILT_FACE_HOLD_MS;
      action.motion = MotionCommand::Center;
      action.enableIdleSway = true;
      break;

    case LocalIntent::Sleep:
      action.id = BehaviorActionId::IntentSleep;
      action.priority = 82;
      action.expression = ExpressionType::Sad;
      action.facePriority = EyeAnimPriority::Alert;
      action.faceHoldMs = HardwareConfig::Behavior::SHAKE_FACE_HOLD_MS;
      action.motion = MotionCommand::SoftListen;
      action.enableIdleSway = false;
      break;

    case LocalIntent::Wake:
      action.id = BehaviorActionId::IntentWake;
      action.priority = 84;
      action.expression = ExpressionType::FaceRecognized;
      action.facePriority = EyeAnimPriority::Alert;
      action.faceHoldMs = HardwareConfig::Behavior::SHAKE_FACE_HOLD_MS;
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

  publishActionEvent(action.id, nowMs);
  diagnostics_.logInfo(kBehaviorEventLog);
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
