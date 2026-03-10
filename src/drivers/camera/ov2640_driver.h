#pragma once

#include "../../interfaces/i_camera.h"

class Ov2640Driver : public ICamera {
public:
  bool init() override;
  bool captureFrame(VisionSnapshot& outSnapshot) override;
  bool isReady() const override;

private:
  bool initRealBackend();
  bool captureRealFrame(VisionSnapshot& outSnapshot);
  bool captureSyntheticFrame(VisionSnapshot& outSnapshot);

  bool ready_ = false;
  bool realBackend_ = false;
  uint16_t lastLuma_ = 0;
  uint16_t motionFiltered_ = 0;
  uint32_t frameId_ = 0;
  uint32_t consecutiveCaptureFails_ = 0;
};
