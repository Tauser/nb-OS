#pragma once

#include "../interfaces/i_camera.h"
#include "../interfaces/i_camera_port.h"

class VisionHAL : public ICameraPort {
public:
  explicit VisionHAL(ICamera& camera);

  bool init() override;
  bool sampleFrame(VisionSnapshot& outSnapshot) override;
  bool isReady() const override;

private:
  ICamera& camera_;
};
