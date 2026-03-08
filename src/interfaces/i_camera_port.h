#pragma once

#include "../models/vision_snapshot.h"

class ICameraPort {
public:
  virtual ~ICameraPort() = default;

  virtual bool init() = 0;
  virtual bool sampleFrame(VisionSnapshot& outSnapshot) = 0;
  virtual bool isReady() const = 0;
};
