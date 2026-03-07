#include "app_factory.h"

#include "../core/config_manager.h"
#include "../core/diagnostics.h"
#include "../core/event_bus.h"
#include "../core/storage_manager.h"
#include "../core/system_manager.h"
#include "../drivers/camera/ov2640_driver.h"
#include "../drivers/display/lovyan_st7789_driver.h"
#include "../drivers/imu/mpu6050_driver.h"
#include "../drivers/touch/touch_driver.h"
#include "../hal/camera_hal.h"
#include "../hal/display_hal.h"
#include "../hal/sensor_hal.h"
#include "../services/face/face_service.h"
#include "../services/interaction/interaction_service.h"
#include "../services/sensor/sensor_service.h"
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

Mpu6050Driver g_imuDriver;
TouchDriver g_touchDriver;
SensorHAL g_sensorHal(g_imuDriver, g_touchDriver);
SensorService g_sensorService(g_sensorHal, g_eventBus);

InteractionService g_interactionService(g_eventBus, g_diagnostics);

SystemManager g_systemManager(
    g_eventBus,
    g_diagnostics,
    g_configManager,
    g_storageManager,
    g_faceService,
    g_visionService,
    g_sensorService);
}

void AppFactory::init() {
  g_interactionService.init();
  g_systemManager.init();
}

void AppFactory::update() {
  g_systemManager.update();
  g_interactionService.update();
}
