#pragma once

#include <stddef.h>
#include <stdint.h>

class IAudioIn {
public:
  virtual ~IAudioIn() = default;

  virtual bool initInput() = 0;
  virtual bool isInputReady() const = 0;
  virtual size_t readSamples(int16_t* monoBuffer, size_t maxSamples, uint32_t timeoutMs) = 0;
};
