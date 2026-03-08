#include "system_manager.h"

#include "../config/feature_flags.h"
#include "../config/hardware_config.h"
#include "../models/event.h"
#include <Arduino.h>

namespace {
void logModule(Diagnostics& diagnostics, const char* moduleName, bool enabled) {
  if (enabled) {
    diagnostics.logInfo(moduleName);
  }
}

float intervalScaleForMode(PowerMode mode, float normalScale, float lowScale, float sleepScale) {
  switch (mode) {
    case PowerMode::LowPower:
      return lowScale;
    case PowerMode::Sleep:
      return sleepScale;
    case PowerMode::Charging:
      return normalScale;
    case PowerMode::Normal:
    default:
      return normalScale;
  }
}
}

SystemManager::SystemManager(EventBus& eventBus,
                             Diagnostics& diagnostics,
                             ConfigManager& configManager,
                             StorageManager& storageManager,
                             IVisualService& visualService,
                             IVisionService& visionService,
                             ISensorHub& sensorHub,
                             IMotion& motionService)
    : eventBus_(eventBus),
      diagnostics_(diagnostics),
      configManager_(configManager),
      storageManager_(storageManager),
      visualService_(visualService),
      visionService_(visionService),
      sensorHub_(sensorHub),
      motionService_(motionService) {}

void SystemManager::init() {
  diagnostics_.begin();
  diagnostics_.printBanner();

  eventBus_.subscribe(EventType::EVT_POWER_MODE_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_SAFE_MODE_CHANGED, this);

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

  if (FeatureFlags::IMU_ENABLED || FeatureFlags::TOUCH_ENABLED) {
    sensorHub_.init();
    logModule(diagnostics_, "[BOOT] Sensor hub enabled", true);
  }

  if (FeatureFlags::SERVO_BUS_ENABLED) {
    motionService_.init();
    logModule(diagnostics_, "[BOOT] Neck motion enabled", true);
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

  unsigned long faceInterval = scaledInterval(
      cfg.faceFrameIntervalMs,
      intervalScaleForMode(powerMode_,
                           1.0f,
                           HardwareConfig::Power::LOW_POWER_FACE_INTERVAL_SCALE,
                           HardwareConfig::Power::SLEEP_FACE_INTERVAL_SCALE));

  unsigned long sensorInterval = scaledInterval(
      cfg.sensorPollIntervalMs,
      intervalScaleForMode(powerMode_,
                           1.0f,
                           HardwareConfig::Power::LOW_POWER_SENSOR_INTERVAL_SCALE,
                           HardwareConfig::Power::SLEEP_SENSOR_INTERVAL_SCALE));

  unsigned long motionInterval = scaledInterval(
      cfg.motionUpdateIntervalMs,
      intervalScaleForMode(powerMode_,
                           1.0f,
                           HardwareConfig::Power::LOW_POWER_MOTION_INTERVAL_SCALE,
                           HardwareConfig::Power::SLEEP_MOTION_INTERVAL_SCALE));

  unsigned long heartbeatInterval = scaledInterval(
      cfg.heartbeatIntervalMs,
      intervalScaleForMode(powerMode_,
                           1.0f,
                           HardwareConfig::Power::LOW_POWER_HEARTBEAT_INTERVAL_SCALE,
                           HardwareConfig::Power::SLEEP_HEARTBEAT_INTERVAL_SCALE));

  if (safeMode_) {
    faceInterval = scaledInterval(faceInterval, HardwareConfig::Recovery::SAFE_MODE_FACE_INTERVAL_SCALE);
    sensorInterval = scaledInterval(sensorInterval, HardwareConfig::Recovery::SAFE_MODE_SENSOR_INTERVAL_SCALE);
    heartbeatInterval = scaledInterval(heartbeatInterval, HardwareConfig::Recovery::SAFE_MODE_HEARTBEAT_INTERVAL_SCALE);
  }

  if (FeatureFlags::DISPLAY_ENABLED && now - lastFrameMs_ >= faceInterval) {
    lastFrameMs_ = now;
    visualService_.update(now);
    publishEvent(EventType::FaceFrameRendered, EventSource::FaceService);
  }

  const bool cameraAllowed = (powerMode_ != PowerMode::Sleep) && !safeMode_;
  if (FeatureFlags::CAMERA_ENABLED && cameraAllowed) {
    visionService_.update();
    publishEvent(EventType::CameraFrameSampled, EventSource::VisionService);
  }

  if ((FeatureFlags::IMU_ENABLED || FeatureFlags::TOUCH_ENABLED) && now - lastSensorPollMs_ >= sensorInterval) {
    lastSensorPollMs_ = now;
    sensorHub_.update(now);
  }

  const bool motionAllowed = (powerMode_ != PowerMode::Sleep) && !safeMode_;
  if (FeatureFlags::SERVO_BUS_ENABLED && motionAllowed && now - lastMotionUpdateMs_ >= motionInterval) {
    lastMotionUpdateMs_ = now;
    motionService_.update(now);
  }

  if (now - lastHeartbeatMs_ >= heartbeatInterval) {
    lastHeartbeatMs_ = now;
    publishEvent(EventType::Heartbeat);
    publishEvent(EventType::EVT_IDLE);
    diagnostics_.printHeartbeat(now, getStateName());
  }
}

RobotState SystemManager::getState() const {
  return state_;
}

void SystemManager::onEvent(const Event& event) {
  if (event.type == EventType::EVT_POWER_MODE_CHANGED) {
    powerMode_ = static_cast<PowerMode>(event.value);

    if (powerMode_ == PowerMode::Sleep) {
      motionService_.center();
    }
    return;
  }

  if (event.type == EventType::EVT_SAFE_MODE_CHANGED) {
    safeMode_ = (event.value != 0);
    if (safeMode_) {
      motionService_.center();
    }
  }
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

unsigned long SystemManager::scaledInterval(unsigned long baseMs, float scale) const {
  if (scale <= 0.0f) {
    return baseMs;
  }

  const unsigned long scaled = static_cast<unsigned long>(static_cast<float>(baseMs) * scale);
  return (scaled == 0UL) ? 1UL : scaled;
}
