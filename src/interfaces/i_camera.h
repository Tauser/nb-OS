#pragma once

class ICamera {
public:
  virtual ~ICamera() = default;
  virtual bool init() = 0;
  virtual bool captureFrame() = 0;
};