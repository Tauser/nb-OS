#include "camera_hal.h"

CameraHAL::CameraHAL(ICamera& camera) : camera_(camera) {}

bool CameraHAL::init() {
  return camera_.init();
}

bool CameraHAL::sampleFrame(VisionSnapshot& outSnapshot) {
  return camera_.captureFrame(outSnapshot);
}

bool CameraHAL::isReady() const {
  return camera_.isReady();
}
