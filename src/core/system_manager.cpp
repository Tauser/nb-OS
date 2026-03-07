#include "system_manager.h"

#include "../config/feature_flags.h"
#include "../models/event.h"
#include <Arduino.h>

namespace {
void logModule(Diagnostics& diagnostics, const char* moduleName, bool enabled) {
  if (enabled) {
    diagnostics.logInfo(moduleName);
  }
}
}

SystemManager::SystemManager(EventBus& eventBus,
                             Diagnostics& diagnostics,
                             ConfigManager& configManager,
                             StorageManager& storageManager,
                             IVisualService& visualService,
                             IVisionService& visionService)
    : eventBus_(eventBus),
      diagnostics_(diagnostics),
      configManager_(configManager),
      storageManager_(storageManager),
      visualService_(visualService),
      visionService_(visionService) {}

void SystemManager::init() {
  diagnostics_.begin();
  diagnostics_.printBanner();

  publishEvent(EventType::BootStarted);

  configManager_.init();

  if (FeatureFlags::STORAGE_SD_ENABLED) {
    storageManager_.init();
    logModule(diagnostics_, "[BOOT] SD storage enabled", true);
  }

  if (FeatureFlags::DISPLAY_ENABLED) {
    visualService_.init();
    logModule(diagnostics_, "[BOOT] Display/Face enabled", true);
  }

  if (FeatureFlags::CAMERA_ENABLED) {
    visionService_.init();
    logModule(diagnostics_, "[BOOT] Camera/Vision enabled", true);
  }

  logModule(diagnostics_, "[BOOT] IMU module planned", FeatureFlags::IMU_ENABLED);
  logModule(diagnostics_, "[BOOT] Touch module planned", FeatureFlags::TOUCH_ENABLED);
  logModule(diagnostics_, "[BOOT] Audio input module planned", FeatureFlags::AUDIO_INPUT_ENABLED);
  logModule(diagnostics_, "[BOOT] Audio output module planned", FeatureFlags::AUDIO_OUTPUT_ENABLED);
  logModule(diagnostics_, "[BOOT] Servo bus module planned", FeatureFlags::SERVO_BUS_ENABLED);

  state_ = RobotState::Idle;
  publishEvent(EventType::BootComplete);
}

void SystemManager::update() {
  const unsigned long now = millis();
  const RobotConfig& cfg = configManager_.get();

  if (FeatureFlags::DISPLAY_ENABLED && now - lastFrameMs_ >= cfg.faceFrameIntervalMs) {
    lastFrameMs_ = now;
    visualService_.update(now);
    publishEvent(EventType::FaceFrameRendered, EventSource::FaceService);
  }

  if (FeatureFlags::CAMERA_ENABLED) {
    visionService_.update();
    publishEvent(EventType::CameraFrameSampled, EventSource::VisionService);
  }

  if (now - lastHeartbeatMs_ >= cfg.heartbeatIntervalMs) {
    lastHeartbeatMs_ = now;
    publishEvent(EventType::Heartbeat);
    diagnostics_.printHeartbeat(now, getStateName());
  }
}

RobotState SystemManager::getState() const {
  return state_;
}

void SystemManager::publishEvent(EventType type, EventSource source, int value) {
  Event event;
  event.type = type;
  event.source = source;
  event.value = value;
  event.timestamp = millis();
  eventBus_.publish(event);
}

const char* SystemManager::getStateName() const {
  switch (state_) {
    case RobotState::Boot:
      return "Boot";
    case RobotState::Idle:
      return "Idle";
    case RobotState::Error:
      return "Error";
    default:
      return "Unknown";
  }
}
