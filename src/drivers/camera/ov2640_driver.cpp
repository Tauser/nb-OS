#include "ov2640_driver.h"

#include <Arduino.h>

bool Ov2640Driver::init() {
  // Local lightweight bootstrap; ready for future esp32-camera backend.
  ready_ = true;
  lastLuma_ = 512;
  frameId_ = 0;
  return true;
}

bool Ov2640Driver::captureFrame(VisionSnapshot& outSnapshot) {
  if (!ready_) {
    return false;
  }

  frameId_++;

  // Synthetic luminance baseline used as placeholder until full OV2640 backend is enabled.
  const uint32_t t = millis();
  const uint16_t wave = static_cast<uint16_t>((t / 17U) % 1024U);
  const uint16_t noise = static_cast<uint16_t>(esp_random() % 80U);
  const uint16_t luma = static_cast<uint16_t>((wave * 7U + noise * 3U) / 10U);

  const int delta = static_cast<int>(luma) - static_cast<int>(lastLuma_);
  const uint16_t motion = static_cast<uint16_t>(delta < 0 ? -delta : delta);

  outSnapshot.valid = true;
  outSnapshot.frameId = frameId_;
  outSnapshot.timestampMs = t;
  outSnapshot.avgLuma = luma;
  outSnapshot.motionScore = motion > 1023U ? 1023U : motion;

  lastLuma_ = luma;
  return true;
}

bool Ov2640Driver::isReady() const {
  return ready_;
}
