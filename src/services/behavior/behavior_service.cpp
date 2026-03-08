#include "behavior_service.h"

#include "../../config/hardware_config.h"
#include "../../models/event.h"

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
}

void BehaviorService::update(unsigned long nowMs) {
  if (context_.lastIdleEvalMs == 0) {
    context_.lastIdleEvalMs = nowMs;
  }

  if (activePriority_ > 0 && nowMs >= activeUntilMs_) {
    activePriority_ = 0;
  }

  if (nowMs - context_.lastIdleEvalMs < HardwareConfig::Behavior::IDLE_EVAL_INTERVAL_MS) {
    return;
  }

  context_.lastIdleEvalMs = nowMs;
  const BehaviorAction emotionalAction = actionFromEmotion(nowMs);
  tryApplyAction(emotionalAction, nowMs);
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
      event.type == EventType::EVT_VOICE_ACTIVITY) {
    context_.lastInteractionMs = nowMs;
  }

  if (event.type == EventType::EVT_IDLE) {
    context_.idleTicks++;
    return;
  }

  const BehaviorAction action = actionFromEvent(event);
  tryApplyAction(action, nowMs);
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
