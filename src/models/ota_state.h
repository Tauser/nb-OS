#pragma once

#include <stdint.h>

enum class OtaStage : uint8_t {
  Idle = 0,
  Checking,
  Ready,
  Applying,
  Applied,
  Blocked,
  Failed
};

struct OtaState {
  OtaStage stage = OtaStage::Idle;
  bool updateAvailable = false;
  bool integrityOk = false;
  bool policyAllowed = false;
  bool safeModeForced = false;

  uint32_t currentVersion = 1;
  uint32_t targetVersion = 1;
  uint32_t packageChecksum = 0;
  int lastErrorCode = 0;
};
