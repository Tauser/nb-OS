#include "inmp441_driver.h"

#include "../../config/hardware_pins.h"
#include <driver/i2s.h>

namespace {
constexpr i2s_port_t kMicPort = I2S_NUM_1;
}

bool Inmp441Driver::init(uint32_t sampleRateHz, uint16_t dmaBufferSamples) {
  if (ready_) {
    return true;
  }

  const i2s_config_t i2sConfig = {
      .mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_RX),
      .sample_rate = sampleRateHz,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
      .communication_format = I2S_COMM_FORMAT_STAND_I2S,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 6,
      .dma_buf_len = dmaBufferSamples,
      .use_apll = false,
      .tx_desc_auto_clear = false,
      .fixed_mclk = 0};

  const i2s_pin_config_t pinConfig = {
      .bck_io_num = HardwarePins::AudioIn::BCLK,
      .ws_io_num = HardwarePins::AudioIn::WS,
      .data_out_num = I2S_PIN_NO_CHANGE,
      .data_in_num = HardwarePins::AudioIn::DATA};

  if (i2s_driver_install(kMicPort, &i2sConfig, 0, nullptr) != ESP_OK) {
    return false;
  }

  if (i2s_set_pin(kMicPort, &pinConfig) != ESP_OK) {
    i2s_driver_uninstall(kMicPort);
    return false;
  }

  i2s_zero_dma_buffer(kMicPort);
  ready_ = true;
  return true;
}

bool Inmp441Driver::isReady() const {
  return ready_;
}

size_t Inmp441Driver::readSamples(int16_t* monoBuffer, size_t maxSamples, uint32_t timeoutMs) {
  if (!ready_ || monoBuffer == nullptr || maxSamples == 0) {
    return 0;
  }

  size_t bytesRead = 0;
  const size_t bytesToRead = maxSamples * sizeof(int16_t);
  const esp_err_t err = i2s_read(
      kMicPort,
      monoBuffer,
      bytesToRead,
      &bytesRead,
      pdMS_TO_TICKS(timeoutMs));

  if (err != ESP_OK || bytesRead == 0) {
    return 0;
  }

  return bytesRead / sizeof(int16_t);
}
