#include "routine_service.h"

#include "../../config/hardware_config.h"
#include "../../models/event.h"
#include "../../models/power_state.h"
#include <Arduino.h>

#ifndef NCOS_SIM_MODE
#define NCOS_SIM_MODE 0
#endif

namespace {
const char* routineStateName(RoutineState state) {
  switch (state) {
    case RoutineState::Idle: return "idle";
    case RoutineState::Attentive: return "attentive";
    case RoutineState::Calm: return "calm";
    case RoutineState::Curious: return "curious";
    case RoutineState::Sleepy: return "sleepy";
    case RoutineState::Bored: return "bored";
    case RoutineState::Resume: return "resume";
    case RoutineState::Charging: return "charging";
    case RoutineState::Listening: return "listening";
    case RoutineState::Rest: return "rest";
    default: return "none";
  }
}
}

RoutineService::RoutineService(EventBus& eventBus,
                               const IEmotionProvider& emotionProvider,
                               IFaceController& faceController,
                               IMotion& motion)
    : eventBus_(eventBus), emotionProvider_(emotionProvider), faceController_(faceController), motion_(motion) {}

void RoutineService::init() {
  eventBus_.subscribe(EventType::EVT_IDLE, this);
  eventBus_.subscribe(EventType::EVT_VOICE_START, this);
  eventBus_.subscribe(EventType::EVT_VOICE_ACTIVITY, this);
  eventBus_.subscribe(EventType::EVT_INTENT_DETECTED, this);
  eventBus_.subscribe(EventType::EVT_POWER_MODE_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_CHARGING_STATE_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_TOUCH, this);

  const unsigned long nowMs = millis();
  lastUpdateMs_ = nowMs;
  lastInteractionMs_ = nowMs;
  lastAutonomyStepMs_ = nowMs;
  lastAttentionRecoveryMs_ = nowMs;
  idleStage_ = IdleAutonomyStage::Attentive;
  setState(RoutineState::Idle, nowMs);
}

void RoutineService::update(unsigned long nowMs) {
  if (nowMs - lastUpdateMs_ < HardwareConfig::Polish::ROUTINE_UPDATE_INTERVAL_MS) {
    return;
  }

  lastUpdateMs_ = nowMs;

  if (state_ == RoutineState::Charging) {
    faceController_.requestExpression(ExpressionType::FaceRecognized, EyeAnimPriority::Critical, 1800);
    motion_.center();
    return;
  }

  if (state_ == RoutineState::Rest) {
    faceController_.requestExpression(ExpressionType::BatteryAlert, EyeAnimPriority::Critical, 1800);
    motion_.center();
    return;
  }

  if (state_ == RoutineState::Listening) {
    faceController_.requestExpression(ExpressionType::FaceRecognized, EyeAnimPriority::Social, 1400);
    motion_.softListen();
    return;
  }

  // Keep sleepy ownership stable to avoid face ping-pong with other idle services.
  if (state_ == RoutineState::Sleepy) {
    faceController_.requestExpression(ExpressionType::BatteryAlert, EyeAnimPriority::Critical, 1800);
    motion_.softListen();
    return;
  }

  updateIdleAutonomy(nowMs);
}

void RoutineService::onEvent(const Event& event) {
  const unsigned long nowMs = (event.timestamp > 0) ? event.timestamp : millis();

  switch (event.type) {
    case EventType::EVT_TOUCH:
      markInteraction(nowMs);
      setState(RoutineState::Resume, nowMs);
      faceController_.requestExpression(ExpressionType::FaceRecognized, EyeAnimPriority::Social, 500);
      motion_.idleSway();
      break;

    case EventType::EVT_VOICE_START:
    case EventType::EVT_VOICE_ACTIVITY:
    case EventType::EVT_INTENT_DETECTED:
      markInteraction(nowMs);
      setState(RoutineState::Listening, nowMs);
      faceController_.requestExpression(ExpressionType::FaceRecognized, EyeAnimPriority::Social, 650);
      motion_.softListen();
      break;

    case EventType::EVT_POWER_MODE_CHANGED: {
      powerMode_ = static_cast<PowerMode>(event.value);
      if (powerMode_ == PowerMode::Charging) {
        setState(RoutineState::Charging, nowMs);
        faceController_.requestExpression(ExpressionType::FaceRecognized, EyeAnimPriority::Critical, 1400);
        motion_.center();
      } else if (powerMode_ == PowerMode::Sleep) {
        setState(RoutineState::Rest, nowMs);
        faceController_.requestExpression(ExpressionType::BatteryAlert, EyeAnimPriority::Critical, 1400);
        motion_.center();
      } else if (state_ == RoutineState::Charging || state_ == RoutineState::Rest) {
        setState(RoutineState::Resume, nowMs);
      }
      break;
    }

    case EventType::EVT_CHARGING_STATE_CHANGED:
      if (event.value != 0 && powerMode_ == PowerMode::Charging) {
        setState(RoutineState::Charging, nowMs);
        faceController_.requestExpression(ExpressionType::FaceRecognized, EyeAnimPriority::Critical, 1400);
        motion_.center();
      } else if (event.value == 0 && state_ == RoutineState::Charging) {
        setState(RoutineState::Resume, nowMs);
      }
      break;

    case EventType::EVT_IDLE:
      if (state_ == RoutineState::Resume || state_ == RoutineState::Listening) {
        setState(RoutineState::Idle, nowMs);
      }
      break;

    default:
      break;
  }
}

void RoutineService::setState(RoutineState state, unsigned long nowMs) {
  if (state_ == state) {
    return;
  }

  state_ = state;
#if NCOS_SIM_MODE
  Serial.print("[ROUTINE_STATE] state=");
  Serial.println(routineStateName(state_));
#endif
  publishRoutineState(nowMs);
}

void RoutineService::publishRoutineState(unsigned long nowMs) {
  Event event;
  event.type = EventType::EVT_ROUTINE_STATE_CHANGED;
  event.source = EventSource::RoutineService;
  event.value = static_cast<int>(state_);
  event.timestamp = nowMs;
  eventBus_.publish(event);
}

void RoutineService::updateIdleAutonomy(unsigned long nowMs) {
  const unsigned long idleForMs = nowMs - lastInteractionMs_;
  const IdleAutonomyStage targetStage = stageForIdle(idleForMs);
  const bool stageChanged = (targetStage != idleStage_);

  if (stageChanged) {
    idleStage_ = targetStage;
  }

  if (idleForMs >= HardwareConfig::Polish::IDLE_ATTENTION_RECOVERY_AFTER_MS &&
      nowMs - lastAttentionRecoveryMs_ >= HardwareConfig::Polish::IDLE_ATTENTION_RECOVERY_INTERVAL_MS) {
    lastAttentionRecoveryMs_ = nowMs;
    ExpressionType recoveryExpr = ExpressionType::FaceRecognized;
    EyeAnimPriority recoveryPriority = EyeAnimPriority::Social;
    unsigned long recoveryHoldMs = 520;
    if (idleStage_ == IdleAutonomyStage::Sleepy) {
      recoveryExpr = ExpressionType::BatteryAlert;
      recoveryPriority = EyeAnimPriority::Idle;
      recoveryHoldMs = 760;
    } else if (idleStage_ == IdleAutonomyStage::Bored) {
      recoveryExpr = ExpressionType::Sad;
      recoveryPriority = EyeAnimPriority::Idle;
      recoveryHoldMs = 760;
    }
    faceController_.requestExpression(recoveryExpr, recoveryPriority, recoveryHoldMs);
    if (nextLeft_) {
      motion_.yawLeft();
    } else {
      motion_.yawRight();
    }
    nextLeft_ = !nextLeft_;
  }

  if (!stageChanged) {
    return;
  }

  lastAutonomyStepMs_ = nowMs;
  applyStage(idleStage_, nowMs, stageChanged);
}

RoutineService::IdleAutonomyStage RoutineService::stageForIdle(unsigned long idleForMs) const {
  const EmotionState& emo = emotionProvider_.getEmotionState();

  if (idleForMs >= HardwareConfig::Polish::IDLE_BORED_MS && emo.valence < HardwareConfig::Homeostasis::ROUTINE_BORED_VALENCE_MAX) {
    return IdleAutonomyStage::Bored;
  }

  const bool allowSleepyByEnergy = (idleForMs >= HardwareConfig::Polish::POST_INTERACTION_AWAKE_MIN_MS);
  if (idleForMs >= HardwareConfig::Polish::IDLE_SLEEPY_MS ||
      (allowSleepyByEnergy && emo.energy <= HardwareConfig::Homeostasis::ROUTINE_SLEEPY_ENERGY_MAX)) {
    return IdleAutonomyStage::Sleepy;
  }

  if (emo.curiosity >= HardwareConfig::Homeostasis::ROUTINE_CURIOUS_MIN || idleForMs >= HardwareConfig::Polish::IDLE_CURIOUS_MS) {
    return IdleAutonomyStage::Curious;
  }

  if (idleForMs >= HardwareConfig::Polish::IDLE_CALM_MS || emo.arousal <= HardwareConfig::Homeostasis::ROUTINE_CALM_AROUSAL_MAX) {
    return IdleAutonomyStage::Calm;
  }

  return IdleAutonomyStage::Attentive;
}
void RoutineService::applyStage(IdleAutonomyStage stage, unsigned long nowMs, bool stageChanged) {
  (void)nowMs;

  switch (stage) {
    case IdleAutonomyStage::Attentive:
      setState(RoutineState::Attentive, nowMs);
      faceController_.requestExpression(ExpressionType::FaceRecognized, EyeAnimPriority::Idle, stageChanged ? 560 : 360);
      motion_.idleSway();
      break;

    case IdleAutonomyStage::Calm:
      setState(RoutineState::Calm, nowMs);
      faceController_.requestExpression(ExpressionType::Neutral, EyeAnimPriority::Idle, stageChanged ? 680 : 460);
      motion_.idleSway();
      break;

    case IdleAutonomyStage::Curious:
      setState(RoutineState::Curious, nowMs);
      faceController_.requestExpression(ExpressionType::Curiosity, EyeAnimPriority::Idle, stageChanged ? 980 : 680);
      if (nextLeft_) {
        motion_.curiousLeft();
      } else {
        motion_.curiousRight();
      }
      nextLeft_ = !nextLeft_;
      break;

    case IdleAutonomyStage::Sleepy:
      setState(RoutineState::Sleepy, nowMs);
      faceController_.requestExpression(ExpressionType::BatteryAlert, EyeAnimPriority::Critical, stageChanged ? 1900 : 1500);
      motion_.softListen();
      break;

    case IdleAutonomyStage::Bored:
      setState(RoutineState::Bored, nowMs);
      faceController_.requestExpression(ExpressionType::Sad, EyeAnimPriority::Idle, stageChanged ? 1450 : 1080);
      if (nextLeft_) {
        motion_.yawLeft();
      } else {
        motion_.yawRight();
      }
      nextLeft_ = !nextLeft_;
      break;

    default:
      break;
  }
}

void RoutineService::markInteraction(unsigned long nowMs) {
  lastInteractionMs_ = nowMs;
  lastAutonomyStepMs_ = nowMs;
  lastAttentionRecoveryMs_ = nowMs;
  idleStage_ = IdleAutonomyStage::Attentive;
}
















