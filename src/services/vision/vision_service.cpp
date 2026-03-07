#include "vision_service.h"

VisionService::VisionService(ICameraPort& cameraPort) : cameraPort_(cameraPort) {}

void VisionService::init() {
  cameraPort_.init();
}

void VisionService::update() {
  cameraPort_.sampleFrame();
}
