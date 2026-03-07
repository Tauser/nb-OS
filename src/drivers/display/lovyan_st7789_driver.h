#pragma once

#include "../../interfaces/i_display.h"
#include <LovyanGFX.hpp>

class LovyanSt7789Driver : public IDisplay {
public:
  LovyanSt7789Driver();

  void init() override;
  void clear() override;
  void present() override;
  void drawEye(int x, int y, int radius, float openness) override;
  void drawPupil(int x, int y, int radius) override;
  void drawText(int x, int y, const char* text) override;

private:
  class DisplayDevice : public lgfx::LGFX_Device {
  public:
    DisplayDevice();

    lgfx::Panel_ST7789 panel_;
    lgfx::Bus_SPI bus_;
  };

  DisplayDevice lcd_;
};