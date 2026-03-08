#pragma once

#include <stddef.h>
#include <stdint.h>

class Inmp441Driver {
public:
  bool init(uint32_t sampleRateHz, uint16_t dmaBufferSamples);
  bool isReady() const;
  size_t readSamples(int16_t* monoBuffer, size_t maxSamples, uint32_t timeoutMs);

private:
  bool ready_ = false;
};
