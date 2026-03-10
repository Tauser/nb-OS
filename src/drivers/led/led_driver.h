#pragma once

#include <stdint.h>

class LedDriver {
public:
  bool init();
  bool setMono(uint8_t intensity);
  bool setRgb(uint8_t red, uint8_t green, uint8_t blue);
  bool off();
  bool isReady() const;

private:
  bool dataAvailable_ = false;
  bool ready_ = false;

  uint8_t brightness_ = 255;
  uint8_t lastRed_ = 0;
  uint8_t lastGreen_ = 0;
  uint8_t lastBlue_ = 0;
};
