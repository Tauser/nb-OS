#include "audio_hal.h"

#include "../config/hardware_config.h"
#include "../utils/math_utils.h"
#include <math.h>

namespace {
constexpr float kTwoPi = 6.28318530717958647692f;
}

AudioHAL::AudioHAL(Max98357aDriver& audioOutDriver, Inmp441Driver& audioInDriver)
    : audioOutDriver_(audioOutDriver), audioInDriver_(audioInDriver) {}

bool AudioHAL::init() {
  return audioOutDriver_.init(
      HardwareConfig::AudioOut::SAMPLE_RATE_HZ,
      HardwareConfig::AudioOut::DMA_BUFFER_SAMPLES);
}

bool AudioHAL::isReady() const {
  return audioOutDriver_.isReady();
}

bool AudioHAL::playTone(uint16_t freqHz, uint16_t durationMs, float amplitude) {
  if (!isReady() || freqHz == 0 || durationMs == 0) {
    return false;
  }

  const uint32_t sampleRate = HardwareConfig::AudioOut::SAMPLE_RATE_HZ;
  const uint32_t totalFrames = (sampleRate * static_cast<uint32_t>(durationMs)) / 1000U;
  const float clampedAmp = MathUtils::clamp(amplitude, 0.0f, 0.95f);

  constexpr size_t kChunkFrames = 128;
  int16_t buffer[kChunkFrames * 2];

  float phase = 0.0f;
  const float phaseStep = (kTwoPi * static_cast<float>(freqHz)) / static_cast<float>(sampleRate);

  uint32_t remaining = totalFrames;
  while (remaining > 0) {
    const size_t frames = remaining > kChunkFrames ? kChunkFrames : static_cast<size_t>(remaining);

    for (size_t i = 0; i < frames; ++i) {
      const float sample = sinf(phase) * clampedAmp;
      const int16_t pcm = static_cast<int16_t>(sample * 32767.0f);
      buffer[(i * 2) + 0] = pcm;
      buffer[(i * 2) + 1] = pcm;

      phase += phaseStep;
      if (phase >= kTwoPi) {
        phase -= kTwoPi;
      }
    }

    if (!audioOutDriver_.writeFrames(buffer, frames, HardwareConfig::AudioOut::WRITE_TIMEOUT_MS)) {
      return false;
    }

    remaining -= static_cast<uint32_t>(frames);
  }

  return true;
}

void AudioHAL::stop() {
  audioOutDriver_.stop();
}

bool AudioHAL::initInput() {
  return audioInDriver_.init(
      HardwareConfig::AudioIn::SAMPLE_RATE_HZ,
      HardwareConfig::AudioIn::DMA_BUFFER_SAMPLES);
}

bool AudioHAL::isInputReady() const {
  return audioInDriver_.isReady();
}

size_t AudioHAL::readSamples(int16_t* monoBuffer, size_t maxSamples, uint32_t timeoutMs) {
  return audioInDriver_.readSamples(monoBuffer, maxSamples, timeoutMs);
}

bool AudioHAL::playNotification() {
  const bool a = playTone(HardwareConfig::AudioOut::NOTIFY_FREQ_A_HZ,
                          HardwareConfig::AudioOut::NOTIFY_TONE_MS,
                          HardwareConfig::AudioOut::DEFAULT_AMPLITUDE);
  const bool b = playTone(HardwareConfig::AudioOut::NOTIFY_FREQ_B_HZ,
                          HardwareConfig::AudioOut::NOTIFY_TONE_MS,
                          HardwareConfig::AudioOut::DEFAULT_AMPLITUDE);
  return a && b;
}
