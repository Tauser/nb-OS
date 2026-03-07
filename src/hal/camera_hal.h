#pragma once

#include "../interfaces/i_camera.h"

class CameraHAL {
public:
  explicit CameraHAL(ICamera& camera);

  bool init();
  bool sampleFrame();

private:
  ICamera& camera_;
};