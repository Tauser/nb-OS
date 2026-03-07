#pragma once

#include "../../interfaces/i_camera_port.h"
#include "../../interfaces/i_vision_service.h"

class VisionService : public IVisionService {
public:
  explicit VisionService(ICameraPort& cameraPort);

  void init() override;
  void update() override;

private:
  ICameraPort& cameraPort_;
};
