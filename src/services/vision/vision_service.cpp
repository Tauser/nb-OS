#include "vision_service.h"

VisionService::VisionService(CameraHAL& cameraHal) : cameraHal_(cameraHal) {}

void VisionService::init() {
  cameraHal_.init();
}

void VisionService::update() {
  cameraHal_.sampleFrame();
}