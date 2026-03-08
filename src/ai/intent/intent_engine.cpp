#include "intent_engine.h"

#include "../../config/hardware_config.h"

bool IntentEngine::init() {
  return true;
}

LocalIntent IntentEngine::classify(const VoiceCommandFrame& frame) const {
  if (frame.durationMs < HardwareConfig::Intent::MIN_COMMAND_MS) {
    return LocalIntent::None;
  }

  if (frame.burstCount >= HardwareConfig::Intent::PHOTO_BURST_MIN) {
    return LocalIntent::Photo;
  }

  return classifySingleBurst(frame);
}

LocalIntent IntentEngine::classifySingleBurst(const VoiceCommandFrame& frame) const {
  if (frame.durationMs <= HardwareConfig::Intent::HELLO_MAX_MS &&
      frame.avgLevel <= HardwareConfig::Intent::HELLO_MAX_LEVEL) {
    return LocalIntent::Hello;
  }

  if (frame.durationMs <= HardwareConfig::Intent::WAKE_MAX_MS &&
      frame.peakLevel >= HardwareConfig::Intent::WAKE_MIN_PEAK_LEVEL) {
    return LocalIntent::Wake;
  }

  if (frame.durationMs >= HardwareConfig::Intent::SLEEP_MIN_MS &&
      frame.avgLevel <= HardwareConfig::Intent::SLEEP_MAX_LEVEL) {
    return LocalIntent::Sleep;
  }

  if (frame.durationMs >= HardwareConfig::Intent::STATUS_MIN_MS &&
      frame.durationMs <= HardwareConfig::Intent::STATUS_MAX_MS) {
    return LocalIntent::Status;
  }

  return LocalIntent::None;
}
