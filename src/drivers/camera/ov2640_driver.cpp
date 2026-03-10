#include "ov2640_driver.h"

#include "../../config/feature_flags.h"
#include "../../config/hardware_config.h"
#include "../../config/hardware_pins.h"

#include <Arduino.h>

#ifndef NCOS_SIM_MODE
#define NCOS_SIM_MODE 0
#endif

#if !NCOS_SIM_MODE
#include <esp_camera.h>
#endif

namespace {

uint16_t normalizeLuma8ToConfig(uint32_t avgLuma8) {
  if (avgLuma8 > 255U) {
    avgLuma8 = 255U;
  }

  return static_cast<uint16_t>((avgLuma8 * HardwareConfig::Vision::LUMA_OUTPUT_MAX) / 255U);
}

uint16_t clampMotion(uint32_t value) {
  if (value > HardwareConfig::Vision::LUMA_OUTPUT_MAX) {
    value = HardwareConfig::Vision::LUMA_OUTPUT_MAX;
  }
  return static_cast<uint16_t>(value);
}

} // namespace

bool Ov2640Driver::init() {
  ready_ = false;
  realBackend_ = false;
  lastLuma_ = 0;
  motionFiltered_ = 0;
  frameId_ = 0;
  consecutiveCaptureFails_ = 0;

  if (FeatureFlags::VISION_REAL_ENABLED) {
    realBackend_ = initRealBackend();
  }

  // Always keep a synthetic fallback path to guarantee rollback safety.
  ready_ = true;
  return true;
}

bool Ov2640Driver::captureFrame(VisionSnapshot& outSnapshot) {
  if (!ready_) {
    return false;
  }

  if (realBackend_) {
    if (captureRealFrame(outSnapshot)) {
      consecutiveCaptureFails_ = 0;
      return true;
    }

    consecutiveCaptureFails_++;
    if (consecutiveCaptureFails_ >= HardwareConfig::Vision::MAX_CONSECUTIVE_CAPTURE_FAILS) {
      realBackend_ = false;
    }
  }

  return captureSyntheticFrame(outSnapshot);
}

bool Ov2640Driver::isReady() const {
  return ready_;
}

bool Ov2640Driver::initRealBackend() {
#if NCOS_SIM_MODE
  return false;
#else
  camera_config_t config{};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;

  config.pin_d0 = HardwarePins::Camera::Y2;
  config.pin_d1 = HardwarePins::Camera::Y3;
  config.pin_d2 = HardwarePins::Camera::Y4;
  config.pin_d3 = HardwarePins::Camera::Y5;
  config.pin_d4 = HardwarePins::Camera::Y6;
  config.pin_d5 = HardwarePins::Camera::Y7;
  config.pin_d6 = HardwarePins::Camera::Y8;
  config.pin_d7 = HardwarePins::Camera::Y9;

  config.pin_xclk = HardwarePins::Camera::XCLK;
  config.pin_pclk = HardwarePins::Camera::PCLK;
  config.pin_vsync = HardwarePins::Camera::VSYNC;
  config.pin_href = HardwarePins::Camera::HREF;
  config.pin_sccb_sda = HardwarePins::Camera::SIOD;
  config.pin_sccb_scl = HardwarePins::Camera::SIOC;
  config.pin_pwdn = HardwarePins::Camera::PWDN;
  config.pin_reset = HardwarePins::Camera::RESET;

  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_GRAYSCALE;
  config.frame_size = FRAMESIZE_QQVGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;

#if defined(CAMERA_GRAB_LATEST)
  config.grab_mode = CAMERA_GRAB_LATEST;
#endif

  if (esp_camera_init(&config) != ESP_OK) {
    return false;
  }

  sensor_t* sensor = esp_camera_sensor_get();
  if (sensor != nullptr) {
    sensor->set_framesize(sensor, FRAMESIZE_QQVGA);
    sensor->set_brightness(sensor, 0);
    sensor->set_contrast(sensor, 0);
  }

  return true;
#endif
}

bool Ov2640Driver::captureRealFrame(VisionSnapshot& outSnapshot) {
#if NCOS_SIM_MODE
  (void)outSnapshot;
  return false;
#else
  const uint32_t startUs = micros();

  camera_fb_t* fb = esp_camera_fb_get();
  if (fb == nullptr || fb->buf == nullptr || fb->len == 0U) {
    if (fb != nullptr) {
      esp_camera_fb_return(fb);
    }
    return false;
  }

  uint64_t sum = 0;
  uint32_t samples = 0;
  const uint32_t stride = (HardwareConfig::Vision::REAL_SAMPLE_STRIDE > 0U)
                              ? HardwareConfig::Vision::REAL_SAMPLE_STRIDE
                              : 1U;

  for (size_t i = 0; i < fb->len; i += stride) {
    sum += fb->buf[i];
    samples++;
  }

  esp_camera_fb_return(fb);

  if (samples == 0U) {
    return false;
  }

  const uint32_t avgLuma8 = static_cast<uint32_t>(sum / samples);
  const uint16_t luma = normalizeLuma8ToConfig(avgLuma8);
  const uint16_t delta = (luma >= lastLuma_) ? (luma - lastLuma_) : (lastLuma_ - luma);

  const uint32_t alpha = HardwareConfig::Vision::MOTION_IIR_ALPHA_PCT;
  motionFiltered_ = static_cast<uint16_t>(((motionFiltered_ * (100U - alpha)) + (delta * alpha)) / 100U);

  frameId_++;
  outSnapshot.valid = true;
  outSnapshot.realBackend = true;
  outSnapshot.frameId = frameId_;
  outSnapshot.timestampMs = millis();
  outSnapshot.captureUs = micros() - startUs;
  outSnapshot.avgLuma = luma;
  outSnapshot.motionScore = clampMotion(motionFiltered_);

  lastLuma_ = luma;
  return true;
#endif
}

bool Ov2640Driver::captureSyntheticFrame(VisionSnapshot& outSnapshot) {
  frameId_++;

  const uint32_t startUs = micros();
  const uint32_t t = millis();
  const uint16_t wave = static_cast<uint16_t>((t / 17U) % 1024U);
  const uint16_t noise = static_cast<uint16_t>(esp_random() % 80U);
  const uint16_t luma = static_cast<uint16_t>((wave * 7U + noise * 3U) / 10U);

  const int delta = static_cast<int>(luma) - static_cast<int>(lastLuma_);
  const uint16_t motion = static_cast<uint16_t>(delta < 0 ? -delta : delta);

  outSnapshot.valid = true;
  outSnapshot.realBackend = false;
  outSnapshot.frameId = frameId_;
  outSnapshot.timestampMs = t;
  outSnapshot.captureUs = micros() - startUs;
  outSnapshot.avgLuma = luma;
  outSnapshot.motionScore = clampMotion(motion);

  lastLuma_ = luma;
  return true;
}
