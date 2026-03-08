#pragma once

#include <stdint.h>

enum class CloudRequestType : uint8_t {
  None = 0,
  VoiceUnknownIntent,
  VisionMotion,
  VisionDark
};

enum class CloudResultCode : uint8_t {
  None = 0,
  Success,
  Failed,
  Timeout,
  SkippedOffline
};
