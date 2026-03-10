#include "led_orchestrator_service.h"

#include <Arduino.h>
#include <cmath>

#include "../../config/hardware_config.h"
#include "../../models/event.h"
#include "../../models/health_snapshot.h"
#include "../../models/routine_state.h"
#include "../../utils/math_utils.h"

namespace {
uint8_t channelMax(uint8_t r, uint8_t g, uint8_t b) {
  uint8_t m = r > g ? r : g;
  return b > m ? b : m;
}

unsigned long eventTimeOrNow(const Event& event) {
  return (event.timestamp > 0) ? static_cast<unsigned long>(event.timestamp) : millis();
}
}

LedOrchestratorService::LedOrchestratorService(EventBus& eventBus, LedHAL& ledHal)
    : eventBus_(eventBus), ledHal_(ledHal) {}

void LedOrchestratorService::init() {
  eventBus_.subscribe(EventType::BootStarted, this);
  eventBus_.subscribe(EventType::BootComplete, this);
  eventBus_.subscribe(EventType::EVT_POWER_STATUS, this);
  eventBus_.subscribe(EventType::EVT_POWER_MODE_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_CHARGING_STATE_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_SAFE_MODE_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_OTA_STATUS, this);
  eventBus_.subscribe(EventType::EVT_HEALTH_ANOMALY, this);
  eventBus_.subscribe(EventType::EVT_SELF_TEST_RESULT, this);

  eventBus_.subscribe(EventType::EVT_VOICE_START, this);
  eventBus_.subscribe(EventType::EVT_VOICE_ACTIVITY, this);
  eventBus_.subscribe(EventType::EVT_VOICE_END, this);
  eventBus_.subscribe(EventType::EVT_INTENT_DETECTED, this);
  eventBus_.subscribe(EventType::EVT_DIALOGUE_RESPONSE, this);
  eventBus_.subscribe(EventType::EVT_ATTENTION_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_BEHAVIOR_ACTION, this);
  eventBus_.subscribe(EventType::EVT_ROUTINE_STATE_CHANGED, this);

  eventBus_.subscribe(EventType::EVT_EMOTION_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_MOOD_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_MOOD_PROFILE_CHANGED, this);

  const unsigned long nowMs = millis();
  stateSinceMs_ = nowMs;
  activePatternCode_ = 0;
  lastFrameMs_ = nowMs;
}

void LedOrchestratorService::update(unsigned long nowMs) {
  if (!ledHal_.isReady()) {
    return;
  }

  const StatusState statusState = resolveStatusState();
  const InteractionState interactionState = resolveInteractionState(nowMs);
  const EmotionLedState emotionState = resolveEmotionState(nowMs);

  uint16_t selectedCode = 0;
  const Pattern selected = selectPattern(statusState, interactionState, emotionState, selectedCode);

  if (selectedCode != activePatternCode_) {
    activePatternCode_ = selectedCode;
    stateSinceMs_ = nowMs;
  }

  const RgbTarget target = samplePattern(selected, nowMs);
  driveSmooth(target, nowMs);
}

void LedOrchestratorService::onEvent(const Event& event) {
  const unsigned long nowMs = eventTimeOrNow(event);

  switch (event.type) {
    case EventType::BootStarted:
      bootComplete_ = false;
      break;

    case EventType::BootComplete:
      bootComplete_ = true;
      hasError_ = false;
      selfTestFailed_ = false;
      break;

    case EventType::EVT_POWER_STATUS:
      if (event.hasTypedPayload(EventPayloadKind::PowerStatus)) {
        batteryPercent_ = MathUtils::clamp(event.payload.powerStatus.batteryPercent, 0, 100);
        charging_ = event.payload.powerStatus.charging;
      } else {
        batteryPercent_ = MathUtils::clamp(event.value, 0, 100);
      }
      break;

    case EventType::EVT_POWER_MODE_CHANGED:
      powerMode_ = static_cast<PowerMode>(event.value);
      break;

    case EventType::EVT_CHARGING_STATE_CHANGED:
      charging_ = (event.value != 0);
      break;

    case EventType::EVT_SAFE_MODE_CHANGED:
      safeMode_ = (event.value != 0);
      break;

    case EventType::EVT_OTA_STATUS:
      otaStage_ = static_cast<OtaStage>(event.value);
      break;

    case EventType::EVT_HEALTH_ANOMALY:
      hasError_ = (event.value != static_cast<int>(HealthAnomalyCode::None));
      break;

    case EventType::EVT_SELF_TEST_RESULT:
      selfTestFailed_ = (event.value != 0);
      if (!selfTestFailed_) {
        hasError_ = false;
      }
      break;

    case EventType::EVT_VOICE_START:
      voiceActive_ = true;
      wakewordUntilMs_ = nowMs + HardwareConfig::LedInteraction::WAKEWORD_HOLD_MS;
      listeningUntilMs_ = nowMs + HardwareConfig::LedInteraction::LISTENING_HOLD_MS;
      processingUntilMs_ = 0;
      break;

    case EventType::EVT_VOICE_ACTIVITY:
      if (!voiceActive_ && nowMs >= wakewordUntilMs_) {
        wakewordUntilMs_ = nowMs + HardwareConfig::LedInteraction::WAKEWORD_HOLD_MS;
      }
      voiceActive_ = true;
      listeningUntilMs_ = nowMs + HardwareConfig::LedInteraction::LISTENING_EXTEND_MS;
      break;

    case EventType::EVT_VOICE_END:
      voiceActive_ = false;
      processingUntilMs_ = nowMs + HardwareConfig::LedInteraction::PROCESSING_HOLD_MS;
      break;

    case EventType::EVT_INTENT_DETECTED:
      intentUntilMs_ = nowMs + HardwareConfig::LedInteraction::INTENT_HOLD_MS;
      processingUntilMs_ = nowMs + HardwareConfig::LedInteraction::POST_INTENT_PROCESSING_HOLD_MS;
      speakingUntilMs_ = nowMs + HardwareConfig::LedInteraction::SPEAKING_HOLD_MS;
      break;

    case EventType::EVT_DIALOGUE_RESPONSE:
      speakingUntilMs_ = nowMs + HardwareConfig::LedInteraction::SPEAKING_HOLD_MS;
      break;

    case EventType::EVT_ATTENTION_CHANGED:
      attentionFocus_ = static_cast<AttentionFocus>(event.value);
      if (attentionFocus_ == AttentionFocus::Internal ||
          attentionFocus_ == AttentionFocus::Voice ||
          attentionFocus_ == AttentionFocus::Touch) {
        attentionPulseUntilMs_ = nowMs + HardwareConfig::LedInteraction::ATTENTION_PULSE_HOLD_MS;
      }
      break;

    case EventType::EVT_BEHAVIOR_ACTION:
      if (attentionFocus_ == AttentionFocus::Internal) {
        attentionPulseUntilMs_ = nowMs + HardwareConfig::LedInteraction::ATTENTION_PULSE_HOLD_MS;
      }
      break;

    case EventType::EVT_ROUTINE_STATE_CHANGED:
      if (static_cast<RoutineState>(event.value) == RoutineState::Listening) {
        listeningUntilMs_ = nowMs + HardwareConfig::LedInteraction::LISTENING_HOLD_MS;
      }
      break;

    case EventType::EVT_MOOD_CHANGED:
      if (event.hasTypedPayload(EventPayloadKind::MoodChanged)) {
        moodValence_ = MathUtils::clamp(event.payload.moodChanged.valence, -1.0f, 1.0f);
      } else {
        moodValence_ = MathUtils::clamp(static_cast<float>(event.value) / 1000.0f, -1.0f, 1.0f);
      }
      break;

    case EventType::EVT_MOOD_PROFILE_CHANGED:
      moodProfile_ = static_cast<MoodProfile>(event.value);
      break;

    case EventType::EVT_EMOTION_CHANGED: {
      const float prevArousal = arousal_;
      if (event.hasTypedPayload(EventPayloadKind::EmotionChanged)) {
        arousal_ = MathUtils::clamp(event.payload.emotionChanged.arousal, 0.0f, 1.0f);
      } else {
        arousal_ = MathUtils::clamp(static_cast<float>(event.value) / 1000.0f, 0.0f, 1.0f);
      }
      if (arousal_ >= HardwareConfig::LedEmotion::SURPRISE_AROUSAL_THRESHOLD &&
          (arousal_ - prevArousal) >= HardwareConfig::LedEmotion::SURPRISE_DELTA_THRESHOLD) {
        surpriseUntilMs_ = nowMs + HardwareConfig::LedEmotion::SURPRISE_FLASH_MS;
      }
      break;
    }

    default:
      break;
  }
}

LedOrchestratorService::StatusState LedOrchestratorService::resolveStatusState() const {
  if (hasError_ || selfTestFailed_ || otaStage_ == OtaStage::Failed) {
    return StatusState::Error;
  }
  if (safeMode_) {
    return StatusState::SafeMode;
  }
  if (otaStage_ == OtaStage::Applying || otaStage_ == OtaStage::Validating || otaStage_ == OtaStage::Precheck || otaStage_ == OtaStage::Writing || otaStage_ == OtaStage::Verifying) {
    return StatusState::UpdateInProgress;
  }
  if (batteryPercent_ <= static_cast<int>(HardwareConfig::LedStatus::LOW_BATTERY_THRESHOLD_PERCENT)) {
    return StatusState::LowBattery;
  }
  if (charging_ || powerMode_ == PowerMode::Charging) {
    return StatusState::Charging;
  }
  if (powerMode_ == PowerMode::Sleep) {
    return StatusState::Sleep;
  }
  if (!bootComplete_) {
    return StatusState::Booting;
  }
  return StatusState::Ready;
}

LedOrchestratorService::InteractionState LedOrchestratorService::resolveInteractionState(unsigned long nowMs) const {
  if (nowMs < wakewordUntilMs_) {
    return InteractionState::WakewordDetected;
  }
  if (voiceActive_ || nowMs < listeningUntilMs_) {
    return InteractionState::Listening;
  }
  if (nowMs < intentUntilMs_) {
    return InteractionState::IntentDetected;
  }
  if (nowMs < processingUntilMs_) {
    return InteractionState::Processing;
  }
  if (nowMs < speakingUntilMs_) {
    return InteractionState::Speaking;
  }
  if (nowMs < attentionPulseUntilMs_) {
    return InteractionState::AttentionPulse;
  }
  return InteractionState::None;
}

LedOrchestratorService::EmotionLedState LedOrchestratorService::resolveEmotionState(unsigned long nowMs) const {
  if (nowMs < surpriseUntilMs_) {
    return EmotionLedState::SurprisedFlash;
  }
  if (moodProfile_ == MoodProfile::Sleepy) {
    return EmotionLedState::SleepyFade;
  }
  if (moodProfile_ == MoodProfile::Curious) {
    return EmotionLedState::CuriousPulse;
  }
  if (moodValence_ >= HardwareConfig::LedEmotion::HAPPY_VALENCE_THRESHOLD ||
      moodProfile_ == MoodProfile::Social ||
      moodProfile_ == MoodProfile::Animated) {
    return EmotionLedState::HappyAccent;
  }
  if (moodProfile_ == MoodProfile::Calm ||
      (moodValence_ > -0.10f && arousal_ < 0.55f && moodProfile_ != MoodProfile::Sensitive)) {
    return EmotionLedState::CalmGlow;
  }
  return EmotionLedState::None;
}

LedOrchestratorService::Pattern LedOrchestratorService::selectPattern(
    StatusState statusState,
    InteractionState interactionState,
    EmotionLedState emotionState,
    uint16_t& outPatternCode) const {
  // Priority order (22E): ERROR > SAFE_MODE > LOW_BATTERY > CHARGING > INTERACTION > EMOTION > IDLE
  // Extended with UPDATE_IN_PROGRESS right below SAFE_MODE for operational safety.
  if (statusState == StatusState::Error) {
    outPatternCode = 1;
    return patternForStatus(StatusState::Error);
  }
  if (statusState == StatusState::SafeMode) {
    outPatternCode = 2;
    return patternForStatus(StatusState::SafeMode);
  }
  if (statusState == StatusState::UpdateInProgress) {
    outPatternCode = 3;
    return patternForStatus(StatusState::UpdateInProgress);
  }
  if (statusState == StatusState::LowBattery) {
    outPatternCode = 4;
    return patternForStatus(StatusState::LowBattery);
  }
  if (statusState == StatusState::Charging) {
    outPatternCode = 5;
    return patternForStatus(StatusState::Charging);
  }
  if (interactionState != InteractionState::None) {
    outPatternCode = static_cast<uint16_t>(100 + static_cast<uint8_t>(interactionState));
    return patternForInteraction(interactionState);
  }
  if (emotionState != EmotionLedState::None && statusState == StatusState::Ready) {
    outPatternCode = static_cast<uint16_t>(200 + static_cast<uint8_t>(emotionState));
    return patternForEmotion(emotionState);
  }

  // IDLE/base layer (ready, sleep, booting)
  outPatternCode = static_cast<uint16_t>(300 + static_cast<uint8_t>(statusState));
  return patternForStatus(statusState);
}

LedOrchestratorService::Pattern LedOrchestratorService::patternForStatus(StatusState state) const {
  switch (state) {
    case StatusState::Booting:
      return Pattern{0, 0, 255, 220, 220, false, 0};

    case StatusState::Ready:
      return Pattern{0, 200, 40, 1000, 0, true, 2600};

    case StatusState::Charging:
      return Pattern{0, 180, 255, 300, 300, false, 0};

    case StatusState::LowBattery:
      return Pattern{255, 80, 0, 220, 260, false, 0};

    case StatusState::Sleep:
      return Pattern{20, 20, 70, 1600, 0, true, 3200};

    case StatusState::Error:
      return Pattern{255, 0, 0, 140, 140, false, 0};

    case StatusState::SafeMode:
      return Pattern{255, 0, 255, 180, 180, false, 0};

    case StatusState::UpdateInProgress:
      return Pattern{180, 255, 0, 120, 120, false, 0};

    default:
      return Pattern{0, 0, 0, 0, 0, false, 0};
  }
}

LedOrchestratorService::Pattern LedOrchestratorService::patternForInteraction(InteractionState state) const {
  switch (state) {
    case InteractionState::Listening:
      return Pattern{40, 130, 255, 120, 120, false, 0};

    case InteractionState::Processing:
      return Pattern{255, 170, 0, 90, 90, false, 0};

    case InteractionState::Speaking:
      return Pattern{0, 255, 120, 160, 90, false, 0};

    case InteractionState::WakewordDetected:
      return Pattern{255, 255, 255, 70, 40, false, 0};

    case InteractionState::IntentDetected:
      return Pattern{160, 255, 40, 140, 70, false, 0};

    case InteractionState::AttentionPulse:
      return Pattern{70, 0, 255, 900, 0, true, 1200};

    case InteractionState::None:
    default:
      return Pattern{0, 0, 0, 0, 0, false, 0};
  }
}

LedOrchestratorService::Pattern LedOrchestratorService::patternForEmotion(EmotionLedState state) const {
  switch (state) {
    case EmotionLedState::CalmGlow:
      return Pattern{25, 90, 120, 1400, 0, true, 2800};

    case EmotionLedState::HappyAccent:
      return Pattern{60, 220, 90, 220, 260, false, 0};

    case EmotionLedState::CuriousPulse:
      return Pattern{70, 130, 255, 700, 0, true, 1400};

    case EmotionLedState::SleepyFade:
      return Pattern{14, 18, 40, 1800, 0, true, 3200};

    case EmotionLedState::SurprisedFlash:
      return Pattern{255, 255, 240, 85, 35, false, 0};

    case EmotionLedState::None:
    default:
      return Pattern{0, 0, 0, 0, 0, false, 0};
  }
}

LedOrchestratorService::RgbTarget LedOrchestratorService::samplePattern(const Pattern& pattern, unsigned long nowMs) const {
  if (pattern.onMs == 0 && pattern.offMs == 0 && !pattern.pulse) {
    return RgbTarget{0, 0, 0};
  }

  if (pattern.pulse && pattern.pulsePeriodMs > 0) {
    const unsigned long phase = (nowMs - stateSinceMs_) % pattern.pulsePeriodMs;
    const float angle = (static_cast<float>(phase) / static_cast<float>(pattern.pulsePeriodMs)) * 6.2831853f;
    const float norm = (sinf(angle) * 0.5f) + 0.5f;
    const float eased = 0.15f + (norm * 0.85f);

    return RgbTarget{
        static_cast<uint8_t>(static_cast<float>(pattern.red) * eased),
        static_cast<uint8_t>(static_cast<float>(pattern.green) * eased),
        static_cast<uint8_t>(static_cast<float>(pattern.blue) * eased)};
  }

  const unsigned long cycle = pattern.onMs + pattern.offMs;
  bool on = true;
  if (cycle > 0) {
    on = ((nowMs - stateSinceMs_) % cycle) < pattern.onMs;
  }

  if (!on) {
    return RgbTarget{0, 0, 0};
  }

  return RgbTarget{pattern.red, pattern.green, pattern.blue};
}

void LedOrchestratorService::driveSmooth(const RgbTarget& target, unsigned long nowMs) {
  const unsigned long dtMs = (lastFrameMs_ > 0 && nowMs > lastFrameMs_) ? (nowMs - lastFrameMs_) : 0;
  lastFrameMs_ = nowMs;

  if (dtMs == 0) {
    outRed_ = static_cast<float>(target.red);
    outGreen_ = static_cast<float>(target.green);
    outBlue_ = static_cast<float>(target.blue);
  } else {
    const float transitionMs = static_cast<float>(HardwareConfig::LedOrchestration::TRANSITION_MS);
    const float alpha = (transitionMs <= 0.0f)
                            ? 1.0f
                            : MathUtils::clamp(static_cast<float>(dtMs) / transitionMs, 0.0f, 1.0f);

    outRed_ += (static_cast<float>(target.red) - outRed_) * alpha;
    outGreen_ += (static_cast<float>(target.green) - outGreen_) * alpha;
    outBlue_ += (static_cast<float>(target.blue) - outBlue_) * alpha;
  }

  const uint8_t r = static_cast<uint8_t>(MathUtils::clamp(outRed_, 0.0f, 255.0f));
  const uint8_t g = static_cast<uint8_t>(MathUtils::clamp(outGreen_, 0.0f, 255.0f));
  const uint8_t b = static_cast<uint8_t>(MathUtils::clamp(outBlue_, 0.0f, 255.0f));

  ledHal_.setRgb8(r, g, b);
  ledHal_.setMono(channelMax(r, g, b));
}

