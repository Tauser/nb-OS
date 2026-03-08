#pragma once
#include "../config/hardware_config.h"

struct RobotConfig {
  unsigned long heartbeatIntervalMs = HardwareConfig::System::HEARTBEAT_INTERVAL_MS;
  unsigned long faceFrameIntervalMs = HardwareConfig::System::FACE_FRAME_INTERVAL_MS;
  unsigned long sensorPollIntervalMs = HardwareConfig::System::SENSOR_POLL_INTERVAL_MS;
  unsigned long motionUpdateIntervalMs = HardwareConfig::System::MOTION_UPDATE_INTERVAL_MS;
};

class ConfigManager {
public:
  void init();
  const RobotConfig& get() const;

private:
  RobotConfig config_;
};
