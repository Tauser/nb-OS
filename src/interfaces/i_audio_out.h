#pragma once

#include <stdint.h>

class IAudioOut {
public:
  virtual ~IAudioOut() = default;

  virtual bool init() = 0;
  virtual bool isReady() const = 0;
  virtual bool playTone(uint16_t freqHz, uint16_t durationMs, float amplitude = 0.20f) = 0;
  virtual void stop() = 0;
};
