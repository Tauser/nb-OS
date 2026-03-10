#pragma once

#include <stdint.h>

struct RollbackMarker {
  static constexpr uint32_t kSchemaVersion = 1;

  uint32_t schemaVersion = kSchemaVersion;
  bool pending = false;
  uint8_t reserved0 = 0;
  uint16_t lastErrorCode = 0;

  uint32_t fromVersion = 0;
  uint32_t targetVersion = 0;
  uint32_t packageChecksum = 0;

  uint32_t createdAtMs = 0;
  uint32_t attemptCount = 0;

  void sanitize() {
    if (schemaVersion != kSchemaVersion) {
      *this = RollbackMarker{};
      return;
    }

    if (!pending) {
      fromVersion = 0;
      targetVersion = 0;
      packageChecksum = 0;
      createdAtMs = 0;
      attemptCount = 0;
      lastErrorCode = 0;
    }
  }
};
