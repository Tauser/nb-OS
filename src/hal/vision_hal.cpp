#include "vision_hal.h"

VisionHAL::VisionHAL(ICamera& camera) : camera_(camera) {}

bool VisionHAL::init() {
  return camera_.init();
}

bool VisionHAL::sampleFrame(VisionSnapshot& outSnapshot) {
  return camera_.captureFrame(outSnapshot);
}

bool VisionHAL::isReady() const {
  return camera_.isReady();
}
