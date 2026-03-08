#include "app_factory.h"
#include <Arduino.h>

#ifndef NCOS_SIM_MODE
#define NCOS_SIM_MODE 0
#endif

#include "../ai/cloud/cloud_router.h"
#include "../ai/dialogue/dialogue_engine.h"
#include "../ai/intent/intent_engine.h"
#include "../ai/vad/vad_engine.h"
#include "../core/config_manager.h"
#include "../core/diagnostics.h"
#include "../core/event_bus.h"
#include "../core/storage_manager.h"
#include "../core/system_manager.h"
#include "../drivers/audio/inmp441_driver.h"
#include "../drivers/audio/max98357a_driver.h"
#include "../drivers/camera/ov2640_driver.h"
#include "../drivers/display/lovyan_st7789_driver.h"
#include "../drivers/imu/mpu6050_driver.h"
#include "../drivers/motion/feetech_bus_driver.h"
#include "../drivers/power/power_driver.h"
#include "../drivers/touch/touch_driver.h"
#include "../hal/audio_hal.h"
#include "../hal/display_hal.h"
#include "../hal/motion_hal.h"
#include "../hal/power_hal.h"
#include "../hal/sensor_hal.h"
#include "../hal/vision_hal.h"
#include "../services/affinity/affinity_service.h"
#include "../services/attention/attention_service.h"
#include "../services/behavior/behavior_service.h"
#include "../services/emotion/emotion_service.h"
#include "../services/engagement/engagement_service.h"
#include "../services/face/face_service.h"
#include "../services/gaze/gaze_service.h"
#include "../services/gesture/gesture_service.h"
#include "../services/health_monitor/health_monitor_service.h"
#include "../services/interaction/interaction_service.h"
#include "../services/memory/memory_service.h"
#include "../services/mood/mood_service.h"
#include "../services/motion/motion_service.h"
#include "../services/motion_sync/motion_sync_service.h"
#include "../services/ota/ota_service.h"
#include "../services/persona/persona_service.h"
#include "../services/power/power_service.h"
#include "../services/routine/routine_service.h"
#include "../services/safe_mode/safe_mode_service.h"
#include "../services/self_test/self_test_service.h"
#include "../services/sensor/sensor_service.h"
#include "../services/sim_test/sim_test_input_service.h"
#include "../services/vision/vision_service.h"
#include "../services/voice/voice_service.h"

namespace {
EventBus g_eventBus;
Diagnostics g_diagnostics;
ConfigManager g_configManager;
StorageManager g_storageManager;

LovyanSt7789Driver g_displayDriver;
DisplayHAL g_displayHal(g_displayDriver);

EmotionService g_emotionService(g_eventBus);
FaceService g_faceService(g_displayHal, g_emotionService);

Ov2640Driver g_cameraDriver;
VisionHAL g_visionHal(g_cameraDriver);
VisionService g_visionService(g_visionHal, g_eventBus);

Mpu6050Driver g_imuDriver;
TouchDriver g_touchDriver;
SensorHAL g_sensorHal(g_imuDriver, g_touchDriver);
SensorService g_sensorService(g_sensorHal, g_eventBus);

FeetechBusDriver g_motionDriver;
MotionHAL g_motionHal(g_motionDriver);
MotionService g_motionService(g_motionHal, g_eventBus, g_emotionService);

Max98357aDriver g_audioOutDriver;
Inmp441Driver g_audioInDriver;
AudioHAL g_audioHal(g_audioOutDriver, g_audioInDriver);
VadEngine g_vadEngine;
IntentEngine g_intentEngine;
DialogueEngine g_dialogueEngine;
VoiceService g_voiceService(g_audioHal, g_audioHal, g_vadEngine, g_intentEngine, g_dialogueEngine, g_eventBus);

PowerDriver g_powerDriver;
PowerHAL g_powerHal(g_powerDriver);
PowerService g_powerService(g_powerHal, g_eventBus);

AttentionService g_attentionService(g_eventBus);
GazeService g_gazeService(g_eventBus, g_faceService, g_motionService);
GestureService g_gestureService(g_eventBus, g_faceService, g_motionService);
MotionSyncService g_motionSyncService(g_eventBus, g_faceService, g_motionService);
RoutineService g_routineService(g_eventBus, g_faceService, g_motionService);
HealthMonitorService g_healthMonitorService(g_eventBus, g_diagnostics);

AffinityService g_affinityService(g_eventBus);
EngagementService g_engagementService(g_eventBus);
MemoryService g_memoryService(g_eventBus, g_storageManager);
MoodService g_moodService(g_eventBus, g_emotionService);
PersonaService g_personaService(g_eventBus);

SafeModeService g_safeModeService(g_eventBus, g_diagnostics, g_motionService);
SelfTestService g_selfTestService(
    g_eventBus,
    g_diagnostics,
    g_faceService,
    g_motionHal,
    g_sensorHal,
    g_storageManager,
    g_powerService);
OTAService g_otaService(g_eventBus, g_diagnostics, g_storageManager, g_configManager, g_powerService);

CloudRouter g_cloudRouter(g_eventBus);

BehaviorService g_behaviorService(g_eventBus, g_emotionService, g_faceService, g_motionService, g_diagnostics);
InteractionService g_interactionService(g_eventBus, g_diagnostics);

#if NCOS_SIM_MODE
SimTestInputService g_simTestInputService(g_eventBus, g_faceService);
#endif

SystemManager g_systemManager(
    g_eventBus,
    g_diagnostics,
    g_configManager,
    g_storageManager,
    g_faceService,
    g_visionService,
    g_sensorService,
    g_motionService);
}

void AppFactory::init() {
  g_interactionService.init();
  g_emotionService.init();
  g_behaviorService.init();
  g_voiceService.init();
  g_powerService.init();

  g_attentionService.init();
  g_gazeService.init();
  g_gestureService.init();
  g_motionSyncService.init();
  g_routineService.init();
  g_healthMonitorService.init();

  g_affinityService.init();
  g_engagementService.init();
  g_memoryService.init();
  g_moodService.init();
  g_personaService.init();

  g_safeModeService.init();
  g_selfTestService.init();
  g_otaService.init();

  g_cloudRouter.init();
  g_systemManager.init();

#if NCOS_SIM_MODE
  g_simTestInputService.init();
#endif
}

void AppFactory::update() {
  const unsigned long now = millis();

  g_powerService.update(now);
  g_systemManager.update();
  g_interactionService.update();
  g_emotionService.update(now);
  g_behaviorService.update(now);
  g_voiceService.update(now);

  g_attentionService.update(now);
  g_gazeService.update(now);
  g_gestureService.update(now);
  g_motionSyncService.update(now);
  g_routineService.update(now);
  g_healthMonitorService.update(now);

  g_affinityService.update(now);
  g_engagementService.update(now);
  g_memoryService.update(now);
  g_moodService.update(now);
  g_personaService.update(now);

  g_selfTestService.update(now);
  g_safeModeService.update(now);
  g_otaService.update(now);

  g_cloudRouter.update(now);

#if NCOS_SIM_MODE
  g_simTestInputService.update(now);
#endif
}


