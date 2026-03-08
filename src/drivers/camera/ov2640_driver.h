#pragma once

#include "../../interfaces/i_camera.h"

class Ov2640Driver : public ICamera {
public:
  bool init() override;
  bool captureFrame(VisionSnapshot& outSnapshot) override;
  bool isReady() const override;

private:
  bool ready_ = false;
  uint16_t lastLuma_ = 0;
  uint32_t frameId_ = 0;
};
