#pragma once

#include "../../core/event_bus.h"
#include "../../hal/led_hal.h"
#include "../../interfaces/i_event_listener.h"
#include "../../models/attention_focus.h"
#include "../../models/mood_profile.h"
#include "../../models/ota_state.h"
#include "../../models/power_state.h"

class LedOrchestratorService : public IEventListener {
public:
  LedOrchestratorService(EventBus& eventBus, LedHAL& ledHal);

  void init();
  void update(unsigned long nowMs);
  void onEvent(const Event& event) override;

private:
  enum class StatusState : uint8_t {
    Booting = 0,
    Ready,
    Charging,
    LowBattery,
    Sleep,
    Error,
    SafeMode,
    UpdateInProgress
  };

  enum class InteractionState : uint8_t {
    None = 0,
    Listening,
    Processing,
    Speaking,
    WakewordDetected,
    IntentDetected,
    AttentionPulse
  };

  enum class EmotionLedState : uint8_t {
    None = 0,
    CalmGlow,
    HappyAccent,
    CuriousPulse,
    SleepyFade,
    SurprisedFlash
  };

  struct Pattern {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    unsigned long onMs;
    unsigned long offMs;
    bool pulse;
    unsigned long pulsePeriodMs;
  };

  struct RgbTarget {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
  };

  StatusState resolveStatusState() const;
  InteractionState resolveInteractionState(unsigned long nowMs) const;
  EmotionLedState resolveEmotionState(unsigned long nowMs) const;

  Pattern selectPattern(StatusState statusState,
                        InteractionState interactionState,
                        EmotionLedState emotionState,
                        uint16_t& outPatternCode) const;

  Pattern patternForStatus(StatusState state) const;
  Pattern patternForInteraction(InteractionState state) const;
  Pattern patternForEmotion(EmotionLedState state) const;

  RgbTarget samplePattern(const Pattern& pattern, unsigned long nowMs) const;
  void driveSmooth(const RgbTarget& target, unsigned long nowMs);

  EventBus& eventBus_;
  LedHAL& ledHal_;

  bool bootComplete_ = false;
  bool safeMode_ = false;
  bool hasError_ = false;
  bool selfTestFailed_ = false;
  bool charging_ = false;
  int batteryPercent_ = 100;
  PowerMode powerMode_ = PowerMode::Normal;
  OtaStage otaStage_ = OtaStage::Idle;

  AttentionFocus attentionFocus_ = AttentionFocus::None;
  bool voiceActive_ = false;
  unsigned long wakewordUntilMs_ = 0;
  unsigned long listeningUntilMs_ = 0;
  unsigned long processingUntilMs_ = 0;
  unsigned long speakingUntilMs_ = 0;
  unsigned long intentUntilMs_ = 0;
  unsigned long attentionPulseUntilMs_ = 0;

  MoodProfile moodProfile_ = MoodProfile::Calm;
  float moodValence_ = 0.0f;
  float arousal_ = 0.35f;
  unsigned long surpriseUntilMs_ = 0;

  uint16_t activePatternCode_ = 0;
  unsigned long stateSinceMs_ = 0;

  float outRed_ = 0.0f;
  float outGreen_ = 0.0f;
  float outBlue_ = 0.0f;
  unsigned long lastFrameMs_ = 0;
};
