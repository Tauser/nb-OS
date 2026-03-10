#pragma once

#include <stdint.h>

namespace FaultSubsystem {
constexpr uint16_t None = 0;
constexpr uint16_t Display = 1u << 0;
constexpr uint16_t Face = 1u << 1;
constexpr uint16_t Motion = 1u << 2;
constexpr uint16_t Sensor = 1u << 3;
constexpr uint16_t Storage = 1u << 4;
constexpr uint16_t Power = 1u << 5;
constexpr uint16_t Vision = 1u << 6;
constexpr uint16_t Voice = 1u << 7;
constexpr uint16_t Cloud = 1u << 8;
constexpr uint16_t Ota = 1u << 9;
constexpr uint16_t SafeMode = 1u << 10;
constexpr uint16_t Health = 1u << 11;
}

enum class FaultSeverity : uint8_t {
  Info = 0,
  Warning,
  Error,
  Critical
};

struct FaultRecord {
  uint32_t timestampMs = 0;
  uint32_t bootCount = 0;
  uint16_t subsystemMask = FaultSubsystem::None;
  uint16_t code = 0;
  uint8_t source = 0;
  uint8_t severity = static_cast<uint8_t>(FaultSeverity::Error);
};

struct FaultHistory {
  static constexpr uint32_t kSchemaVersion = 1;
  static constexpr uint16_t kMaxRecords = 24;

  uint32_t schemaVersion = kSchemaVersion;
  uint16_t count = 0;
  FaultRecord records[kMaxRecords]{};

  void sanitize() {
    if (schemaVersion != kSchemaVersion) {
      schemaVersion = kSchemaVersion;
      count = 0;
      for (uint16_t i = 0; i < kMaxRecords; ++i) {
        records[i] = FaultRecord{};
      }
      return;
    }

    if (count > kMaxRecords) {
      count = kMaxRecords;
    }
  }

  void push(const FaultRecord& record) {
    sanitize();

    if (count < kMaxRecords) {
      records[count++] = record;
      return;
    }

    for (uint16_t i = 1; i < kMaxRecords; ++i) {
      records[i - 1] = records[i];
    }
    records[kMaxRecords - 1] = record;
  }
};
