#include "routine_service.h"

#include "../../config/feature_flags.h"
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
                               IActionOrchestrator& actionOrchestrator,
                               IFaceController& faceController,
                               IMotion& motion)
    : eventBus_(eventBus),
      emotionProvider_(emotionProvider),
      actionOrchestrator_(actionOrchestrator),
      faceController_(faceController),
      motion_(motion) {}

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
    submitRoutineAction(ExpressionType::FaceRecognized,
                        EyeAnimPriority::Critical,
                        1800,
                        ActionMotionCommand::Center,
                        95,
                        "routine_charging",
                        nowMs);
    return;
  }

  if (state_ == RoutineState::Rest) {
    submitRoutineAction(ExpressionType::BatteryAlert,
                        EyeAnimPriority::Critical,
                        1800,
                        ActionMotionCommand::Center,
                        95,
                        "routine_rest",
                        nowMs);
    return;
  }

  if (state_ == RoutineState::Listening) {
    submitRoutineAction(ExpressionType::FaceRecognized,
                        EyeAnimPriority::Social,
                        1400,
                        ActionMotionCommand::SoftListen,
                        75,
                        "routine_listening",
                        nowMs);
    return;
  }

  if (state_ == RoutineState::Sleepy) {
    submitRoutineAction(ExpressionType::BatteryAlert,
                        EyeAnimPriority::Critical,
                        1800,
                        ActionMotionCommand::SoftListen,
                        85,
                        "routine_sleepy",
                        nowMs);
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
      submitRoutineAction(ExpressionType::FaceRecognized,
                          EyeAnimPriority::Social,
                          500,
                          ActionMotionCommand::IdleSway,
                          70,
                          "routine_touch_resume",
                          nowMs);
      break;

    case EventType::EVT_VOICE_START:
    case EventType::EVT_VOICE_ACTIVITY:
    case EventType::EVT_INTENT_DETECTED:
      markInteraction(nowMs);
      setState(RoutineState::Listening, nowMs);
      submitRoutineAction(ExpressionType::FaceRecognized,
                          EyeAnimPriority::Social,
                          650,
                          ActionMotionCommand::SoftListen,
                          78,
                          "routine_voice_listening",
                          nowMs);
      break;

    case EventType::EVT_POWER_MODE_CHANGED: {
      powerMode_ = static_cast<PowerMode>(event.value);
      if (powerMode_ == PowerMode::Charging) {
        setState(RoutineState::Charging, nowMs);
        submitRoutineAction(ExpressionType::FaceRecognized,
                            EyeAnimPriority::Critical,
                            1400,
                            ActionMotionCommand::Center,
                            95,
                            "routine_power_charging",
                            nowMs);
      } else if (powerMode_ == PowerMode::Sleep) {
        setState(RoutineState::Rest, nowMs);
        submitRoutineAction(ExpressionType::BatteryAlert,
                            EyeAnimPriority::Critical,
                            1400,
                            ActionMotionCommand::Center,
                            95,
                            "routine_power_sleep",
                            nowMs);
      } else if (state_ == RoutineState::Charging || state_ == RoutineState::Rest) {
        setState(RoutineState::Resume, nowMs);
      }
      break;
    }

    case EventType::EVT_CHARGING_STATE_CHANGED:
      if (event.value != 0 && powerMode_ == PowerMode::Charging) {
        setState(RoutineState::Charging, nowMs);
        submitRoutineAction(ExpressionType::FaceRecognized,
                            EyeAnimPriority::Critical,
                            1400,
                            ActionMotionCommand::Center,
                            95,
                            "routine_charging_state_on",
                            nowMs);
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

    submitRoutineAction(recoveryExpr,
                        recoveryPriority,
                        recoveryHoldMs,
                        nextLeft_ ? ActionMotionCommand::YawLeft : ActionMotionCommand::YawRight,
                        52,
                        "routine_attention_recovery",
                        nowMs);
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
  switch (stage) {
    case IdleAutonomyStage::Attentive:
      setState(RoutineState::Attentive, nowMs);
      submitRoutineAction(ExpressionType::FaceRecognized,
                          EyeAnimPriority::Idle,
                          stageChanged ? 560 : 360,
                          ActionMotionCommand::IdleSway,
                          34,
                          "routine_idle_attentive",
                          nowMs);
      break;

    case IdleAutonomyStage::Calm:
      setState(RoutineState::Calm, nowMs);
      submitRoutineAction(ExpressionType::Neutral,
                          EyeAnimPriority::Idle,
                          stageChanged ? 680 : 460,
                          ActionMotionCommand::IdleSway,
                          33,
                          "routine_idle_calm",
                          nowMs);
      break;

    case IdleAutonomyStage::Curious:
      setState(RoutineState::Curious, nowMs);
      submitRoutineAction(ExpressionType::Curiosity,
                          EyeAnimPriority::Idle,
                          stageChanged ? 980 : 680,
                          nextLeft_ ? ActionMotionCommand::CuriousLeft : ActionMotionCommand::CuriousRight,
                          42,
                          "routine_idle_curious",
                          nowMs);
      nextLeft_ = !nextLeft_;
      break;

    case IdleAutonomyStage::Sleepy:
      setState(RoutineState::Sleepy, nowMs);
      submitRoutineAction(ExpressionType::BatteryAlert,
                          EyeAnimPriority::Critical,
                          stageChanged ? 1900 : 1500,
                          ActionMotionCommand::SoftListen,
                          85,
                          "routine_idle_sleepy",
                          nowMs);
      break;

    case IdleAutonomyStage::Bored:
      setState(RoutineState::Bored, nowMs);
      submitRoutineAction(ExpressionType::Sad,
                          EyeAnimPriority::Idle,
                          stageChanged ? 1450 : 1080,
                          nextLeft_ ? ActionMotionCommand::YawLeft : ActionMotionCommand::YawRight,
                          36,
                          "routine_idle_bored",
                          nowMs);
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

bool RoutineService::submitRoutineAction(ExpressionType expression,
                                         EyeAnimPriority facePriority,
                                         unsigned long holdMs,
                                         ActionMotionCommand motion,
                                         uint8_t priority,
                                         const char* reason,
                                         unsigned long nowMs) {
  if (FeatureFlags::ACTION_ORCHESTRATOR_ENABLED) {
    ActionIntent faceIntent;
    faceIntent.target = ActionTargetChannel::Face;
    faceIntent.priority = priority;
    faceIntent.holdMs = holdMs;
    faceIntent.source = EventSource::RoutineService;
    faceIntent.reason = reason;
    faceIntent.face.expression = expression;
    faceIntent.face.animPriority = facePriority;
    const bool faceAccepted = actionOrchestrator_.submitIntent(faceIntent, nowMs);

    bool motionAccepted = true;
    if (motion != ActionMotionCommand::None) {
      ActionIntent motionIntent;
      motionIntent.target = ActionTargetChannel::Motion;
      motionIntent.priority = priority;
      motionIntent.holdMs = holdMs;
      motionIntent.source = EventSource::RoutineService;
      motionIntent.reason = reason;
      motionIntent.motion.command = motion;
      motionAccepted = actionOrchestrator_.submitIntent(motionIntent, nowMs);
    }

    return faceAccepted || motionAccepted;
  }

  faceController_.requestExpression(expression, facePriority, holdMs);
  switch (motion) {
    case ActionMotionCommand::Center:
      motion_.center();
      break;
    case ActionMotionCommand::IdleSway:
      motion_.idleSway();
      break;
    case ActionMotionCommand::CuriousLeft:
      motion_.curiousLeft();
      break;
    case ActionMotionCommand::CuriousRight:
      motion_.curiousRight();
      break;
    case ActionMotionCommand::SoftListen:
      motion_.softListen();
      break;
    case ActionMotionCommand::YawLeft:
      motion_.yawLeft();
      break;
    case ActionMotionCommand::YawRight:
      motion_.yawRight();
      break;
    case ActionMotionCommand::TiltLeft:
      motion_.tiltLeft();
      break;
    case ActionMotionCommand::TiltRight:
      motion_.tiltRight();
      break;
    case ActionMotionCommand::None:
    default:
      break;
  }

  return true;
}
