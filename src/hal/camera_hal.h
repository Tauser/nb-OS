#pragma once

#include "../interfaces/i_camera.h"
#include "../interfaces/i_camera_port.h"

class CameraHAL : public ICameraPort {
public:
  explicit CameraHAL(ICamera& camera);

  bool init() override;
  bool sampleFrame() override;

private:
  ICamera& camera_;
};
