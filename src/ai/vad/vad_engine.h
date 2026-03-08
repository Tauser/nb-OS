#pragma once

#include <stddef.h>
#include <stdint.h>

class VadEngine {
public:
  bool init(uint16_t sampleRateHz, uint16_t frameSamples);
  bool process(const int16_t* samples, size_t sampleCount);
  bool isVoiceActive() const;

private:
  float frameRms(const int16_t* samples, size_t sampleCount) const;

  bool initialized_ = false;
  bool voiceActive_ = false;
  float noiseFloor_ = 0.0f;
  uint16_t sampleRateHz_ = 0;
  uint16_t frameSamples_ = 0;
  uint8_t activeFrames_ = 0;
  uint8_t silentFrames_ = 0;
};
