#pragma once

#include "../../interfaces/i_display.h"
#include <LovyanGFX.hpp>

#ifndef NCOS_SIM_MODE
#define NCOS_SIM_MODE 0
#endif

#if NCOS_SIM_MODE
using LgfxPanelType = lgfx::Panel_ILI9341;
#else
using LgfxPanelType = lgfx::Panel_ST7789;
#endif

class LovyanSt7789Driver : public IDisplay {
public:
  LovyanSt7789Driver();

  void init() override;
  void clear() override;
  void clearRect(int x, int y, int w, int h) override;
  void present() override;
  void drawEye(int x,
               int y,
               int radius,
               float openness,
               float tiltDeg,
               float squashY,
               float stretchX,
               float roundness,
               float upperLid,
               float lowerLid) override;
  void drawEyeShape(int x,
                    int y,
                    int radius,
                    float openness,
                    float tiltDeg,
                    float squashY,
                    float stretchX,
                    float upperLid,
                    float lowerLid,
                    const ResolvedEyeShape& shape) override;
  void drawPupil(int x, int y, int radius) override;
  void drawText(int x, int y, const char* text) override;

private:
  class DisplayDevice : public lgfx::LGFX_Device {
  public:
    DisplayDevice();

    LgfxPanelType panel_;
    lgfx::Bus_SPI bus_;
  };

  void drawLegacyEye(int x,
                     int y,
                     int radius,
                     float openness,
                     float tiltDeg,
                     float squashY,
                     float stretchX,
                     float roundness,
                     float upperLid,
                     float lowerLid);

  DisplayDevice lcd_;
};
