#pragma once

#include "../drivers/led/led_driver.h"
#include <stdint.h>

class LedHAL {
public:
  explicit LedHAL(LedDriver& driver);

  bool init();
  bool setIntensity(float normalized);
  bool setMono(uint8_t intensity);
  bool setRgb(float redNorm, float greenNorm, float blueNorm);
  bool setRgb8(uint8_t red, uint8_t green, uint8_t blue);
  bool off();
  bool isReady() const;

private:
  static uint8_t toByte(float normalized);

  LedDriver& driver_;
};
