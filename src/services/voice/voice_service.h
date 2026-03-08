#pragma once

#include "../../ai/dialogue/dialogue_engine.h"
#include "../../ai/intent/intent_engine.h"
#include "../../ai/vad/vad_engine.h"
#include "../../core/event_bus.h"
#include "../../interfaces/i_audio_in.h"
#include "../../interfaces/i_audio_out.h"
#include "../../interfaces/i_event_listener.h"
#include "../../models/intent_types.h"
#include "../../models/persona_profile.h"

class VoiceService : public IEventListener {
public:
  VoiceService(IAudioOut& audioOut,
               IAudioIn& audioIn,
               VadEngine& vadEngine,
               IntentEngine& intentEngine,
               DialogueEngine& dialogueEngine,
               EventBus& eventBus);

  void init();
  void update(unsigned long nowMs);
  void onEvent(const Event& event) override;

private:
  void publishVoiceEvent(EventType type, unsigned long nowMs, int value = 0);
  int computeFrameLevel(const int16_t* samples, size_t sampleCount) const;
  void publishIntent(LocalIntent intent, unsigned long nowMs);
  void publishDialogue(LocalIntent intent, unsigned long nowMs);
  void resetCommandFrame();

  IAudioOut& audioOut_;
  IAudioIn& audioIn_;
  VadEngine& vadEngine_;
  IntentEngine& intentEngine_;
  DialogueEngine& dialogueEngine_;
  EventBus& eventBus_;

  bool outputReady_ = false;
  bool inputReady_ = false;
  bool voiceActive_ = false;
  bool prefersCalmStyle_ = false;
  PersonaTone personaTone_ = PersonaTone::Warm;

  unsigned long lastSoundMs_ = 0;
  unsigned long lastVoiceActivityPublishMs_ = 0;
  unsigned long voiceStartMs_ = 0;
  unsigned long lastVoiceEndMs_ = 0;

  uint32_t levelAccum_ = 0;
  uint16_t levelPeak_ = 0;
  uint16_t levelFrames_ = 0;
  uint8_t burstCount_ = 1;
};
