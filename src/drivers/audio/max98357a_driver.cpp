#include "max98357a_driver.h"

#include "../../config/hardware_pins.h"
#include <driver/i2s.h>

namespace {
constexpr i2s_port_t kAudioPort = I2S_NUM_0;
}

bool Max98357aDriver::init(uint32_t sampleRateHz, uint16_t dmaBufferSamples) {
  if (ready_) {
    return true;
  }

  const i2s_config_t i2sConfig = {
      .mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_TX),
      .sample_rate = sampleRateHz,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
      .communication_format = I2S_COMM_FORMAT_STAND_I2S,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 6,
      .dma_buf_len = dmaBufferSamples,
      .use_apll = false,
      .tx_desc_auto_clear = true,
      .fixed_mclk = 0};

  const i2s_pin_config_t pinConfig = {
      .bck_io_num = HardwarePins::AudioOut::BCLK,
      .ws_io_num = HardwarePins::AudioOut::WS,
      .data_out_num = HardwarePins::AudioOut::DATA,
      .data_in_num = I2S_PIN_NO_CHANGE};

  if (i2s_driver_install(kAudioPort, &i2sConfig, 0, nullptr) != ESP_OK) {
    return false;
  }

  if (i2s_set_pin(kAudioPort, &pinConfig) != ESP_OK) {
    i2s_driver_uninstall(kAudioPort);
    return false;
  }

  i2s_zero_dma_buffer(kAudioPort);
  ready_ = true;
  sampleRateHz_ = sampleRateHz;
  return true;
}

bool Max98357aDriver::isReady() const {
  return ready_;
}

bool Max98357aDriver::writeFrames(const int16_t* interleavedStereo, size_t frameCount, uint32_t timeoutMs) {
  if (!ready_ || interleavedStereo == nullptr || frameCount == 0) {
    return false;
  }

  const size_t bytesToWrite = frameCount * sizeof(int16_t) * 2;
  size_t bytesWritten = 0;
  const esp_err_t err = i2s_write(
      kAudioPort,
      interleavedStereo,
      bytesToWrite,
      &bytesWritten,
      pdMS_TO_TICKS(timeoutMs));

  return err == ESP_OK && bytesWritten == bytesToWrite;
}

void Max98357aDriver::stop() {
  if (!ready_) {
    return;
  }

  i2s_zero_dma_buffer(kAudioPort);
}

