#pragma once

#include "../interfaces/i_display.h"
#include "../interfaces/i_display_port.h"

class DisplayHAL : public IDisplayPort {
public:
  explicit DisplayHAL(IDisplay& display);

  void init() override;
  void clearFrame() override;
  void clearRect(int x, int y, int w, int h) override;
  void drawEye(int x,
               int y,
               int radius,
               float openness,
               float tiltDeg,
               float squashY,
               float stretchX,
               float upperLid,
               float lowerLid) override;
  void drawPupil(int x, int y, int radius) override;
  void drawText(int x, int y, const char* text) override;
  void present() override;

private:
  IDisplay& display_;
};
