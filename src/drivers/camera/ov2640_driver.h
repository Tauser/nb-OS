#pragma once

#include "../../interfaces/i_camera.h"

class Ov2640Driver : public ICamera {
public:
  bool init() override;
  bool captureFrame() override;
};