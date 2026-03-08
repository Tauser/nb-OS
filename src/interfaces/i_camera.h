#pragma once

#include "../models/vision_snapshot.h"

class ICamera {
public:
  virtual ~ICamera() = default;
  virtual bool init() = 0;
  virtual bool captureFrame(VisionSnapshot& outSnapshot) = 0;
  virtual bool isReady() const = 0;
};
