#include "config_manager.h"

void ConfigManager::init() {
}

const RobotConfig& ConfigManager::get() const {
  return config_;
}
