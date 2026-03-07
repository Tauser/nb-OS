#include "camera_hal.h"

CameraHAL::CameraHAL(ICamera& camera) : camera_(camera) {}

bool CameraHAL::init() {
  return camera_.init();
}

bool CameraHAL::sampleFrame() {
  return camera_.captureFrame();
}