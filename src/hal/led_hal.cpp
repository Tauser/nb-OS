#include "led_hal.h"

#include "../utils/math_utils.h"

LedHAL::LedHAL(LedDriver& driver)
    : driver_(driver) {}

bool LedHAL::init() {
  return driver_.init();
}

bool LedHAL::setIntensity(float normalized) {
  return driver_.setMono(toByte(normalized));
}

bool LedHAL::setMono(uint8_t intensity) {
  return driver_.setMono(intensity);
}

bool LedHAL::setRgb(float redNorm, float greenNorm, float blueNorm) {
  return driver_.setRgb(toByte(redNorm), toByte(greenNorm), toByte(blueNorm));
}

bool LedHAL::setRgb8(uint8_t red, uint8_t green, uint8_t blue) {
  return driver_.setRgb(red, green, blue);
}

bool LedHAL::off() {
  return driver_.off();
}

bool LedHAL::isReady() const {
  return driver_.isReady();
}

uint8_t LedHAL::toByte(float normalized) {
  const float clamped = MathUtils::clamp(normalized, 0.0f, 1.0f);
  return static_cast<uint8_t>(clamped * 255.0f);
}
