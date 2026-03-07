#include "display_hal.h"

DisplayHAL::DisplayHAL(IDisplay& display) : display_(display) {}

void DisplayHAL::init() {
  display_.init();
}

void DisplayHAL::clearFrame() {
  display_.clear();
}

void DisplayHAL::drawEye(int x, int y, int radius, float openness) {
  display_.drawEye(x, y, radius, openness);
}

void DisplayHAL::drawPupil(int x, int y, int radius) {
  display_.drawPupil(x, y, radius);
}

void DisplayHAL::drawText(int x, int y, const char* text) {
  display_.drawText(x, y, text);
}

void DisplayHAL::present() {
  display_.present();
}