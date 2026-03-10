#include "led_driver.h"

#include "../../config/hardware_pins.h"
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#ifndef NCOS_SIM_MODE
#define NCOS_SIM_MODE 0
#endif

namespace {
constexpr uint16_t kPixelCount = 2;
Adafruit_NeoPixel* g_strip = nullptr;

uint8_t scaleByBrightness(uint8_t value, uint8_t brightness) {
  const uint16_t scaled = static_cast<uint16_t>(value) * static_cast<uint16_t>(brightness);
  return static_cast<uint8_t>(scaled / 255U);
}

void showSolid(uint8_t red, uint8_t green, uint8_t blue) {
  if (g_strip == nullptr) {
    return;
  }

  for (uint16_t i = 0; i < kPixelCount; ++i) {
    g_strip->setPixelColor(i, g_strip->Color(red, green, blue));
  }
  g_strip->show();
}
}

bool LedDriver::init() {
  dataAvailable_ = (HardwarePins::Led::DATA >= 0);
  ready_ = dataAvailable_;

#if NCOS_SIM_MODE
  if (!ready_) {
    // Keep simulation alive even if LED wiring is temporarily absent.
    ready_ = true;
  }
#endif

  if (dataAvailable_) {
    if (g_strip != nullptr) {
      delete g_strip;
      g_strip = nullptr;
    }

    g_strip = new Adafruit_NeoPixel(
        kPixelCount,
        static_cast<uint16_t>(HardwarePins::Led::DATA),
        NEO_GRB + NEO_KHZ800);

    g_strip->begin();
    g_strip->setBrightness(255);
    g_strip->clear();
    g_strip->show();
  }

  if (ready_) {
    off();
  }

  return ready_;
}

bool LedDriver::setMono(uint8_t intensity) {
  if (!ready_) {
    return false;
  }

  brightness_ = intensity;
  return setRgb(lastRed_, lastGreen_, lastBlue_);
}

bool LedDriver::setRgb(uint8_t red, uint8_t green, uint8_t blue) {
  if (!ready_) {
    return false;
  }

  lastRed_ = red;
  lastGreen_ = green;
  lastBlue_ = blue;

  if (!dataAvailable_) {
    return true;
  }

  const uint8_t r = scaleByBrightness(red, brightness_);
  const uint8_t g = scaleByBrightness(green, brightness_);
  const uint8_t b = scaleByBrightness(blue, brightness_);

  showSolid(r, g, b);
  return true;
}

bool LedDriver::off() {
  if (!ready_) {
    return false;
  }

  lastRed_ = 0;
  lastGreen_ = 0;
  lastBlue_ = 0;

  if (dataAvailable_) {
    showSolid(0, 0, 0);
  }

  return true;
}

bool LedDriver::isReady() const {
  return ready_;
}
