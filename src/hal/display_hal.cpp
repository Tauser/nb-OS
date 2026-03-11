#include "display_hal.h"

DisplayHAL::DisplayHAL(IDisplay& display) : display_(display) {}

void DisplayHAL::init() {
  display_.init();
}

void DisplayHAL::clearFrame() {
  display_.clear();
}

void DisplayHAL::clearRect(int x, int y, int w, int h) {
  display_.clearRect(x, y, w, h);
}

void DisplayHAL::drawEye(int x,
                         int y,
                         int radius,
                         float openness,
                         float tiltDeg,
                         float squashY,
                         float stretchX,
                         float roundness,
                         float upperLid,
                         float lowerLid) {
  display_.drawEye(x, y, radius, openness, tiltDeg, squashY, stretchX, roundness, upperLid, lowerLid);
}

void DisplayHAL::drawEyeShape(int x,
                              int y,
                              int radius,
                              float openness,
                              float tiltDeg,
                              float squashY,
                              float stretchX,
                              float upperLid,
                              float lowerLid,
                              const ResolvedEyeShape& shape) {
  display_.drawEyeShape(x, y, radius, openness, tiltDeg, squashY, stretchX, upperLid, lowerLid, shape);
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
