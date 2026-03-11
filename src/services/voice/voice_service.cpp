#include "voice_service.h"

#include "../../config/feature_flags.h"
#include "../../config/hardware_config.h"
#include "../../models/attention_focus.h"
#include "../../models/cloud_types.h"
#include "../../models/event.h"
#include "../../models/face_sync_cue.h"

VoiceService::VoiceService(IAudioOut& audioOut,
                           IAudioIn& audioIn,
                           VadEngine& vadEngine,
                           IntentEngine& intentEngine,
                           DialogueEngine& dialogueEngine,
                           EventBus& eventBus)
    : audioOut_(audioOut),
      audioIn_(audioIn),
      vadEngine_(vadEngine),
      intentEngine_(intentEngine),
      dialogueEngine_(dialogueEngine),
      eventBus_(eventBus) {}

void VoiceService::init() {
  intentEngine_.init();
  dialogueEngine_.init();

  if (FeatureFlags::AUDIO_OUTPUT_ENABLED) {
    outputReady_ = audioOut_.init();
    if (outputReady_) {
      eventBus_.subscribe(EventType::EVT_TOUCH, this);
      eventBus_.subscribe(EventType::EVT_FALL, this);
      eventBus_.subscribe(EventType::BootComplete, this);
      eventBus_.subscribe(EventType::EVT_PERSONA_UPDATED, this);
      eventBus_.subscribe(EventType::EVT_PREFERENCE_UPDATED, this);
      eventBus_.subscribe(EventType::EVT_FACE_SYNC_CUE, this);
    }
  }

  if (FeatureFlags::AUDIO_INPUT_ENABLED) {
    inputReady_ = audioIn_.initInput();
    if (inputReady_) {
      vadEngine_.init(
          static_cast<uint16_t>(HardwareConfig::AudioIn::SAMPLE_RATE_HZ),
          static_cast<uint16_t>(HardwareConfig::AudioIn::FRAME_SAMPLES));
    }
  }
}

void VoiceService::update(unsigned long nowMs) {
  if (!inputReady_) {
    return;
  }

  constexpr size_t kFrameSamples = HardwareConfig::AudioIn::FRAME_SAMPLES;
  int16_t frame[kFrameSamples];

  const size_t read = audioIn_.readSamples(
      frame,
      HardwareConfig::AudioIn::FRAME_SAMPLES,
      HardwareConfig::AudioIn::READ_TIMEOUT_MS);

  if (read == 0) {
    return;
  }

  const int level = computeFrameLevel(frame, read);
  const bool activeNow = vadEngine_.process(frame, read);

  if (activeNow && !voiceActive_) {
    voiceActive_ = true;
    voiceStartMs_ = nowMs;

    if (lastVoiceEndMs_ > 0 && (nowMs - lastVoiceEndMs_) <= HardwareConfig::Voice::BURST_GAP_MS) {
      burstCount_++;
    } else {
      burstCount_ = 1;
    }

    resetCommandFrame();
    levelAccum_ = static_cast<uint32_t>(level);
    levelPeak_ = static_cast<uint16_t>(level);
    levelFrames_ = 1;

    publishVoiceEvent(EventType::EVT_VOICE_START, nowMs, level);
    lastVoiceActivityPublishMs_ = nowMs;
    return;
  }

  if (!activeNow && voiceActive_) {
    voiceActive_ = false;
    lastVoiceEndMs_ = nowMs;

    const unsigned long durationMs = nowMs - voiceStartMs_;
    const uint16_t avgLevel = levelFrames_ > 0
                                  ? static_cast<uint16_t>(levelAccum_ / static_cast<uint32_t>(levelFrames_))
                                  : 0;

    VoiceCommandFrame cmd;
    cmd.durationMs = static_cast<uint16_t>(durationMs > 65535UL ? 65535UL : durationMs);
    cmd.avgLevel = avgLevel;
    cmd.peakLevel = levelPeak_;
    cmd.burstCount = burstCount_;

    const LocalIntent intent = intentEngine_.classify(cmd);
    if (intent != LocalIntent::None) {
      publishIntent(intent, nowMs);
      publishDialogue(intent, nowMs);
    } else if (FeatureFlags::CLOUD_ENABLED) {
      Event cloudReq;
      cloudReq.type = EventType::EVT_CLOUD_REQUEST;
      cloudReq.source = EventSource::VoiceService;
      cloudReq.value = static_cast<int>(CloudRequestType::VoiceUnknownIntent);
      cloudReq.timestamp = nowMs;
      eventBus_.publish(cloudReq);
    }

    publishVoiceEvent(EventType::EVT_VOICE_END, nowMs, avgLevel);
    return;
  }

  if (activeNow) {
    levelAccum_ += static_cast<uint32_t>(level);
    levelFrames_++;
    if (level > levelPeak_) {
      levelPeak_ = static_cast<uint16_t>(level);
    }
  }

  if (activeNow && nowMs - lastVoiceActivityPublishMs_ >= HardwareConfig::Voice::ACTIVITY_PUBLISH_INTERVAL_MS) {
    lastVoiceActivityPublishMs_ = nowMs;
    publishVoiceEvent(EventType::EVT_VOICE_ACTIVITY, nowMs, level);
  }
}

void VoiceService::onEvent(const Event& event) {
  if (event.type == EventType::EVT_PERSONA_UPDATED) {
    personaTone_ = static_cast<PersonaTone>(event.value);
    return;
  }

  if (event.type == EventType::EVT_PREFERENCE_UPDATED) {
    prefersCalmStyle_ = (event.value == static_cast<int>(AttentionFocus::Voice));
    return;
  }

  if (!outputReady_) {
    return;
  }

  if (event.timestamp - lastSoundMs_ < HardwareConfig::AudioOut::SFX_COOLDOWN_MS) {
    return;
  }

  switch (event.type) {
    case EventType::BootComplete:
      audioOut_.playTone(HardwareConfig::AudioOut::BOOT_FREQ_HZ,
                         HardwareConfig::AudioOut::BOOT_TONE_MS,
                         HardwareConfig::AudioOut::DEFAULT_AMPLITUDE);
      lastSoundMs_ = event.timestamp;
      break;

    case EventType::EVT_TOUCH:
      audioOut_.playTone(HardwareConfig::AudioOut::TOUCH_FREQ_HZ,
                         HardwareConfig::AudioOut::TOUCH_TONE_MS,
                         HardwareConfig::AudioOut::DEFAULT_AMPLITUDE);
      lastSoundMs_ = event.timestamp;
      break;

    case EventType::EVT_FACE_SYNC_CUE: {
      const FaceSyncCue cue = static_cast<FaceSyncCue>(event.value);
      switch (cue) {
        case FaceSyncCue::ClipWakeUp:
          audioOut_.playTone(HardwareConfig::Dialogue::WAKE_TONE_HZ,
                             70,
                             HardwareConfig::AudioOut::DEFAULT_AMPLITUDE * 0.82f);
          lastSoundMs_ = event.timestamp;
          break;

        case FaceSyncCue::ClipGoToSleep:
          audioOut_.playTone(HardwareConfig::Dialogue::SLEEP_TONE_HZ,
                             90,
                             HardwareConfig::AudioOut::DEFAULT_AMPLITUDE * 0.78f);
          lastSoundMs_ = event.timestamp;
          break;

        case FaceSyncCue::ClipHappyAck:
          audioOut_.playTone(HardwareConfig::Dialogue::HELLO_TONE_HZ,
                             55,
                             HardwareConfig::AudioOut::DEFAULT_AMPLITUDE * 0.80f);
          lastSoundMs_ = event.timestamp;
          break;

        case FaceSyncCue::ClipAttentionRecovery:
          audioOut_.playTone(HardwareConfig::Dialogue::STATUS_TONE_HZ,
                             58,
                             HardwareConfig::AudioOut::DEFAULT_AMPLITUDE * 0.72f);
          lastSoundMs_ = event.timestamp;
          break;

        default:
          break;
      }
      break;
    }
    case EventType::EVT_FALL:
      audioOut_.playTone(HardwareConfig::AudioOut::ALERT_FREQ_HZ,
                         HardwareConfig::AudioOut::ALERT_TONE_MS,
                         HardwareConfig::AudioOut::DEFAULT_AMPLITUDE);
      lastSoundMs_ = event.timestamp;
      break;

    default:
      break;
  }
}

void VoiceService::publishVoiceEvent(EventType type, unsigned long nowMs, int value) {
  Event event;
  event.type = type;
  event.source = EventSource::VoiceService;
  event.value = value;
  event.timestamp = nowMs;
  eventBus_.publish(event);
}

int VoiceService::computeFrameLevel(const int16_t* samples, size_t sampleCount) const {
  if (samples == nullptr || sampleCount == 0) {
    return 0;
  }

  long long acc = 0;
  for (size_t i = 0; i < sampleCount; ++i) {
    int v = samples[i];
    if (v < 0) {
      v = -v;
    }
    acc += v;
  }

  const long long meanAbs = acc / static_cast<long long>(sampleCount);
  return static_cast<int>((meanAbs * 1000LL) / 32767LL);
}

void VoiceService::publishIntent(LocalIntent intent, unsigned long nowMs) {
  Event event;
  event.type = EventType::EVT_INTENT_DETECTED;
  event.source = EventSource::IntentEngine;
  event.value = static_cast<int>(intent);
  event.timestamp = nowMs;
  eventBus_.publish(event);
}

void VoiceService::publishDialogue(LocalIntent intent, unsigned long nowMs) {
  const DialogueReply reply = dialogueEngine_.buildReply(intent);

  if (outputReady_ && reply.toneHz > 0 && reply.toneMs > 0) {
    int toneHz = reply.toneHz;
    int toneMs = reply.toneMs;

    if (personaTone_ == PersonaTone::Playful) {
      toneHz += 80;
    } else if (personaTone_ == PersonaTone::Calm || prefersCalmStyle_) {
      toneHz -= 60;
      toneMs += 25;
    }

    audioOut_.playTone(static_cast<unsigned int>(toneHz > 100 ? toneHz : 100),
                       static_cast<unsigned int>(toneMs > 30 ? toneMs : 30),
                       HardwareConfig::AudioOut::DEFAULT_AMPLITUDE);
    lastSoundMs_ = nowMs;
  }

  Event event;
  event.type = EventType::EVT_DIALOGUE_RESPONSE;
  event.source = EventSource::DialogueEngine;
  event.value = static_cast<int>(intent);
  event.timestamp = nowMs;
  eventBus_.publish(event);
}

void VoiceService::resetCommandFrame() {
  levelAccum_ = 0;
  levelPeak_ = 0;
  levelFrames_ = 0;
}



