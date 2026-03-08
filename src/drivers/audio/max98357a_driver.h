#pragma once

#include <stddef.h>
#include <stdint.h>

class Max98357aDriver {
public:
  bool init(uint32_t sampleRateHz, uint16_t dmaBufferSamples);
  bool isReady() const;

  bool writeFrames(const int16_t* interleavedStereo, size_t frameCount, uint32_t timeoutMs);
  void stop();

private:
  bool ready_ = false;
  uint32_t sampleRateHz_ = 0;
};
