#pragma once

#include "../interfaces/i_display.h"

class DisplayHAL {
public:
  explicit DisplayHAL(IDisplay& display);

  void init();
  void clearFrame();
  void drawEye(int x, int y, int radius, float openness);
  void drawPupil(int x, int y, int radius);
  void drawText(int x, int y, const char* text);
  void present();

private:
  IDisplay& display_;
};