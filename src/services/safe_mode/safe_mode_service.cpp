#include "safe_mode_service.h"

#include "../../config/feature_flags.h"
#include "../../models/event.h"
#include "../../models/fault_record.h"
#include "../../models/health_snapshot.h"
#include <Arduino.h>

namespace {
constexpr uint16_t kReasonSelfTest = 0x1001;
constexpr uint16_t kReasonHealth = 0x1002;
}

SafeModeService::SafeModeService(EventBus& eventBus,
                                 Diagnostics& diagnostics,
                                 StorageManager& storageManager,
                                 IMotion& motion)
    : eventBus_(eventBus),
      diagnostics_(diagnostics),
      storageManager_(storageManager),
      motion_(motion) {}

void SafeModeService::init() {
  eventBus_.subscribe(EventType::EVT_SELF_TEST_RESULT, this);
  eventBus_.subscribe(EventType::EVT_HEALTH_ANOMALY, this);
}

void SafeModeService::update(unsigned long nowMs) {
  (void)nowMs;
}

void SafeModeService::onEvent(const Event& event) {
  if (event.type == EventType::EVT_SELF_TEST_RESULT) {
    if (event.value != 0) {
      const uint16_t failureMask = static_cast<uint16_t>(event.value);
      const uint16_t degradedMask = degradedMaskFromSelfTest(failureMask);
      enterSafeMode(event.timestamp, degradedMask, kReasonSelfTest, "self-test failure");
    }
    return;
  }

  if (event.type == EventType::EVT_HEALTH_ANOMALY) {
    const HealthAnomalyCode anomaly = static_cast<HealthAnomalyCode>(event.value);
    if (anomaly == HealthAnomalyCode::HeartbeatTimeout ||
        anomaly == HealthAnomalyCode::LowMemory) {
      const uint16_t degradedMask = degradedMaskFromHealthAnomaly(event.value);
      enterSafeMode(event.timestamp, degradedMask, kReasonHealth, "runtime health anomaly");
    }
  }
}

bool SafeModeService::isActive() const {
  return active_;
}

uint16_t SafeModeService::degradedMask() const {
  return degradedMask_;
}

void SafeModeService::enterSafeMode(unsigned long nowMs,
                                    uint16_t degradedMask,
                                    uint16_t reasonCode,
                                    const char* reason) {
  if (active_) {
    if (FeatureFlags::ROBUST_RECOVERY_ENABLED) {
      degradedMask_ = static_cast<uint16_t>(degradedMask_ | degradedMask);
    }
    return;
  }

  active_ = true;
  degradedMask_ = FeatureFlags::ROBUST_RECOVERY_ENABLED ? degradedMask : DegradeNone;
  motion_.center();

  Event event;
  event.type = EventType::EVT_SAFE_MODE_CHANGED;
  event.source = EventSource::SafeModeService;
  event.value = 1;
  event.timestamp = nowMs;
  eventBus_.publish(event);

  if (FeatureFlags::ROBUST_RECOVERY_ENABLED) {
    FaultRecord record;
    record.timestampMs = nowMs;
    record.bootCount = storageManager_.bootCount();
    record.subsystemMask = FaultSubsystem::SafeMode;
    if (degradedMask_ & DegradeDisplay) record.subsystemMask |= FaultSubsystem::Display;
    if (degradedMask_ & DegradeFace) record.subsystemMask |= FaultSubsystem::Face;
    if (degradedMask_ & DegradeMotion) record.subsystemMask |= FaultSubsystem::Motion;
    if (degradedMask_ & DegradeSensor) record.subsystemMask |= FaultSubsystem::Sensor;
    if (degradedMask_ & DegradeStorage) record.subsystemMask |= FaultSubsystem::Storage;
    if (degradedMask_ & DegradePower) record.subsystemMask |= FaultSubsystem::Power;
    if (degradedMask_ & DegradeVision) record.subsystemMask |= FaultSubsystem::Vision;
    if (degradedMask_ & DegradeVoice) record.subsystemMask |= FaultSubsystem::Voice;
    if (degradedMask_ & DegradeCloud) record.subsystemMask |= FaultSubsystem::Cloud;
    if (degradedMask_ & DegradeOta) record.subsystemMask |= FaultSubsystem::Ota;
    record.code = reasonCode;
    record.source = static_cast<uint8_t>(EventSource::SafeModeService);
    record.severity = static_cast<uint8_t>(FaultSeverity::Critical);
    storageManager_.appendFaultRecord(record);
  }

  diagnostics_.logError("[SAFE_MODE] enabled");
  diagnostics_.logError(reason);
}

uint16_t SafeModeService::degradedMaskFromSelfTest(uint16_t failureMask) const {
  uint16_t out = DegradeNone;

  // SelfTestService::FailureBit mapping.
  if (failureMask & (1u << 0)) out |= DegradeDisplay;
  if (failureMask & (1u << 1)) out |= DegradeFace;
  if (failureMask & (1u << 2)) out |= DegradeMotion;
  if (failureMask & (1u << 3)) out |= DegradeSensor;
  if (failureMask & (1u << 4)) out |= DegradeStorage;
  if (failureMask & (1u << 5)) out |= DegradePower;

  return out;
}

uint16_t SafeModeService::degradedMaskFromHealthAnomaly(int anomalyCode) const {
  const HealthAnomalyCode anomaly = static_cast<HealthAnomalyCode>(anomalyCode);

  if (anomaly == HealthAnomalyCode::HeartbeatTimeout) {
    return static_cast<uint16_t>(DegradeVision | DegradeCloud | DegradeVoice);
  }

  if (anomaly == HealthAnomalyCode::LowMemory) {
    return static_cast<uint16_t>(DegradeVision | DegradeVoice);
  }

  return DegradeNone;
}
