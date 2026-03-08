#include "vad_engine.h"

#include "../../config/hardware_config.h"
#include "../../utils/math_utils.h"
#include <math.h>

bool VadEngine::init(uint16_t sampleRateHz, uint16_t frameSamples) {
  sampleRateHz_ = sampleRateHz;
  frameSamples_ = frameSamples;
  noiseFloor_ = 0.0f;
  activeFrames_ = 0;
  silentFrames_ = 0;
  voiceActive_ = false;
  initialized_ = (sampleRateHz_ > 0 && frameSamples_ > 0);
  return initialized_;
}

bool VadEngine::process(const int16_t* samples, size_t sampleCount) {
  if (!initialized_ || samples == nullptr || sampleCount == 0) {
    return false;
  }

  const float rms = frameRms(samples, sampleCount);

  if (noiseFloor_ <= 0.0f) {
    noiseFloor_ = rms;
  } else {
    const float alpha = voiceActive_ ? HardwareConfig::VAD::NOISE_ALPHA_ACTIVE
                                     : HardwareConfig::VAD::NOISE_ALPHA_IDLE;
    noiseFloor_ = (noiseFloor_ * (1.0f - alpha)) + (rms * alpha);
  }

  const float threshold = noiseFloor_ + HardwareConfig::VAD::ENERGY_THRESHOLD_DELTA;
  const bool speakingNow = rms >= threshold;

  if (speakingNow) {
    activeFrames_ = static_cast<uint8_t>(MathUtils::clamp<int>(activeFrames_ + 1, 0, 255));
    silentFrames_ = 0;
  } else {
    silentFrames_ = static_cast<uint8_t>(MathUtils::clamp<int>(silentFrames_ + 1, 0, 255));
    activeFrames_ = 0;
  }

  if (!voiceActive_ && activeFrames_ >= HardwareConfig::VAD::START_CONSECUTIVE_FRAMES) {
    voiceActive_ = true;
    silentFrames_ = 0;
  } else if (voiceActive_ && silentFrames_ >= HardwareConfig::VAD::END_CONSECUTIVE_FRAMES) {
    voiceActive_ = false;
    activeFrames_ = 0;
  }

  return voiceActive_;
}

bool VadEngine::isVoiceActive() const {
  return voiceActive_;
}

float VadEngine::frameRms(const int16_t* samples, size_t sampleCount) const {
  if (samples == nullptr || sampleCount == 0) {
    return 0.0f;
  }

  double acc = 0.0;
  for (size_t i = 0; i < sampleCount; ++i) {
    const float x = static_cast<float>(samples[i]) / 32768.0f;
    acc += static_cast<double>(x * x);
  }

  const double mean = acc / static_cast<double>(sampleCount);
  return static_cast<float>(sqrt(mean));
}
