#pragma once

#include "../../hal/camera_hal.h"

class VisionService {
public:
  explicit VisionService(CameraHAL& cameraHal);

  void init();
  void update();

private:
  CameraHAL& cameraHal_;
};