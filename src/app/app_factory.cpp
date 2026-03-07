#include "app_factory.h"

#include "../core/system_manager.h"
#include "../core/event_bus.h"
#include "../core/diagnostics.h"
#include "../core/config_manager.h"
#include "../core/storage_manager.h"
#include "../drivers/display/lovyan_st7789_driver.h"
#include "../drivers/camera/ov2640_driver.h"
#include "../hal/display_hal.h"
#include "../hal/camera_hal.h"
#include "../services/face/face_service.h"
#include "../services/vision/vision_service.h"

namespace {
  EventBus g_eventBus;
  Diagnostics g_diagnostics;
  ConfigManager g_configManager;
  StorageManager g_storageManager;

  LovyanSt7789Driver g_displayDriver;
  DisplayHAL g_displayHal(g_displayDriver);
  FaceService g_faceService(g_displayHal);

  Ov2640Driver g_cameraDriver;
  CameraHAL g_cameraHal(g_cameraDriver);
  VisionService g_visionService(g_cameraHal);

  SystemManager g_systemManager(
    g_eventBus,
    g_diagnostics,
    g_configManager,
    g_storageManager,
    g_faceService,
    g_visionService
  );
}

void AppFactory::init() {
  g_systemManager.init();
}

void AppFactory::update() {
  g_systemManager.update();
}