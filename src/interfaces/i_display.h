#pragma once

class IDisplay {
public:
  virtual ~IDisplay() = default;

  virtual void init() = 0;
  virtual void clear() = 0;
  virtual void clearRect(int x, int y, int w, int h) = 0;
  virtual void present() = 0;

  virtual void drawEye(int x,
                       int y,
                       int radius,
                       float openness,
                       float tiltDeg,
                       float squashY,
                       float stretchX,
                       float roundness,
                       float upperLid,
                       float lowerLid) = 0;
  virtual void drawPupil(int x, int y, int radius) = 0;
  virtual void drawText(int x, int y, const char* text) = 0;
};



