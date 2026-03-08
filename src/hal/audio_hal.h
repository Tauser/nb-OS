#pragma once

#include "../drivers/audio/inmp441_driver.h"
#include "../drivers/audio/max98357a_driver.h"
#include "../interfaces/i_audio_in.h"
#include "../interfaces/i_audio_out.h"

class AudioHAL : public IAudioOut, public IAudioIn {
public:
  AudioHAL(Max98357aDriver& audioOutDriver, Inmp441Driver& audioInDriver);

  bool init() override;
  bool isReady() const override;
  bool playTone(uint16_t freqHz, uint16_t durationMs, float amplitude = 0.20f) override;
  void stop() override;

  bool initInput() override;
  bool isInputReady() const override;
  size_t readSamples(int16_t* monoBuffer, size_t maxSamples, uint32_t timeoutMs) override;

  bool playNotification();

private:
  Max98357aDriver& audioOutDriver_;
  Inmp441Driver& audioInDriver_;
};
