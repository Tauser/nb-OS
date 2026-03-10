#include "ota_service.h"

#include "../../config/feature_flags.h"
#include "../../config/hardware_config.h"
#include "../../models/event.h"
#include "../../models/fault_record.h"
#include <Arduino.h>

namespace {
constexpr int kErrPolicyBlocked = 1;
constexpr int kErrIntegrity = 2;
constexpr int kErrRollbackPending = 3;
constexpr int kErrRollbackMarker = 4;
constexpr int kErrVerifyFailed = 5;
}

OTAService::OTAService(EventBus& eventBus,
                       Diagnostics& diagnostics,
                       StorageManager& storageManager,
                       ConfigManager& configManager,
                       IPower& powerService)
    : eventBus_(eventBus),
      diagnostics_(diagnostics),
      storageManager_(storageManager),
      configManager_(configManager),
      powerService_(powerService) {}

void OTAService::init() {
  eventBus_.subscribe(EventType::EVT_OTA_CHECK_REQUEST, this);
  eventBus_.subscribe(EventType::EVT_OTA_APPLY_REQUEST, this);
  eventBus_.subscribe(EventType::EVT_POWER_MODE_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_SAFE_MODE_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_SELF_TEST_RESULT, this);

  const unsigned long nowMs = millis();
  lastCheckMs_ = nowMs;
  lastStatusPublishMs_ = nowMs;

  if (FeatureFlags::ROBUST_RECOVERY_ENABLED) {
    handleRollbackMarkerAtBoot(nowMs);
  }

  publishStatus(nowMs);
}

void OTAService::update(unsigned long nowMs) {
  const RobotConfig& cfg = configManager_.get();
  if (!cfg.otaEnabled) {
    state_.stage = OtaStage::Blocked;
    state_.policyAllowed = false;
    publishStatus(nowMs);
    return;
  }

  if (state_.stage == OtaStage::Writing) {
    if (nowMs - applyStartMs_ >= HardwareConfig::Ota::APPLY_DURATION_MS) {
      state_.stage = OtaStage::Verifying;
      verifyStartMs_ = nowMs;
      publishStatus(nowMs);
    }
    return;
  }

  if (state_.stage == OtaStage::Verifying) {
    if (nowMs - verifyStartMs_ >= HardwareConfig::Ota::VERIFY_DURATION_MS) {
      finishApply(nowMs);
    }
    return;
  }

  if (state_.stage == OtaStage::Applying) {
    // Legacy path
    if (nowMs - applyStartMs_ >= HardwareConfig::Ota::APPLY_DURATION_MS) {
      finishApply(nowMs);
    }
    return;
  }

  if (nowMs - lastCheckMs_ >= cfg.otaCheckIntervalMs) {
    evaluateStagedPackage(nowMs);
    lastCheckMs_ = nowMs;
  }

  if (cfg.otaAutoApply && state_.stage == OtaStage::Ready) {
    applyRequested_ = true;
  }

  if (applyRequested_) {
    if (FeatureFlags::ROBUST_RECOVERY_ENABLED) {
      if (!validatePackage(nowMs)) {
        applyRequested_ = false;
        return;
      }

      if (!runPrecheck(nowMs)) {
        applyRequested_ = false;
        return;
      }

      startApply(nowMs);
      applyRequested_ = false;
      return;
    }

    if (!policyAllowsApply()) {
      state_.stage = OtaStage::Blocked;
      state_.policyAllowed = false;
      state_.lastErrorCode = kErrPolicyBlocked;
      publishStatus(nowMs);
      applyRequested_ = false;
      diagnostics_.logError("[OTA] apply blocked by policy");
      return;
    }

    if (!state_.integrityOk) {
      failApply(kErrIntegrity, nowMs, false);
      applyRequested_ = false;
      return;
    }

    startApply(nowMs);
    applyRequested_ = false;
    return;
  }

  if (nowMs - lastStatusPublishMs_ >= HardwareConfig::Ota::STATUS_PUBLISH_INTERVAL_MS) {
    publishStatus(nowMs);
  }
}

void OTAService::onEvent(const Event& event) {
  switch (event.type) {
    case EventType::EVT_OTA_CHECK_REQUEST: {
      if (event.value > static_cast<int>(state_.currentVersion)) {
        const uint32_t target = static_cast<uint32_t>(event.value);
        storageManager_.stageOtaPackage(target, makeIntegrityToken(target));
      }
      evaluateStagedPackage(event.timestamp);
      lastCheckMs_ = event.timestamp;
      break;
    }

    case EventType::EVT_OTA_APPLY_REQUEST:
      applyRequested_ = true;
      break;

    case EventType::EVT_POWER_MODE_CHANGED:
      if (state_.stage == OtaStage::Ready || state_.stage == OtaStage::Blocked) {
        state_.policyAllowed = policyAllowsApply();
      }
      break;

    case EventType::EVT_SAFE_MODE_CHANGED:
      safeModeActive_ = (event.value != 0);
      if (safeModeActive_ &&
          (state_.stage == OtaStage::Applying || state_.stage == OtaStage::Writing || state_.stage == OtaStage::Verifying)) {
        failApply(kErrPolicyBlocked, event.timestamp, FeatureFlags::ROBUST_RECOVERY_ENABLED);
      }
      break;

    case EventType::EVT_SELF_TEST_RESULT:
      selfTestPassed_ = (event.value == 0);
      break;

    default:
      break;
  }
}

const OtaState& OTAService::getState() const {
  return state_;
}

bool OTAService::policyAllowsApply() const {
  if (!selfTestPassed_ || safeModeActive_) {
    return false;
  }

  const PowerState& power = powerService_.getPowerState();
  const bool batteryOk = power.batteryPercent >= HardwareConfig::Ota::MIN_BATTERY_PERCENT || power.charging;

  if (!batteryOk) {
    return false;
  }

  if (HardwareConfig::Ota::REQUIRE_CHARGING_TO_APPLY && !power.charging) {
    return false;
  }

  return power.mode == PowerMode::Normal || power.mode == PowerMode::Charging;
}

void OTAService::evaluateStagedPackage(unsigned long nowMs) {
  state_.stage = OtaStage::Checking;

  uint32_t stagedVersion = 0;
  uint32_t stagedChecksum = 0;
  if (!storageManager_.getStagedOtaPackage(stagedVersion, stagedChecksum)) {
    state_.updateAvailable = false;
    state_.integrityOk = false;
    state_.targetVersion = state_.currentVersion;
    state_.packageChecksum = 0;
    state_.policyAllowed = policyAllowsApply();
    state_.stage = OtaStage::Idle;
    publishStatus(nowMs);
    return;
  }

  state_.targetVersion = stagedVersion;
  state_.packageChecksum = stagedChecksum;
  state_.updateAvailable = (stagedVersion > state_.currentVersion);
  state_.integrityOk = (stagedChecksum == makeIntegrityToken(stagedVersion));
  state_.policyAllowed = policyAllowsApply();

  if (!state_.updateAvailable) {
    state_.stage = OtaStage::Idle;
  } else if (!state_.integrityOk) {
    state_.stage = OtaStage::Failed;
    state_.lastErrorCode = kErrIntegrity;
  } else if (!state_.policyAllowed) {
    state_.stage = OtaStage::Blocked;
    state_.lastErrorCode = kErrPolicyBlocked;
  } else {
    state_.stage = OtaStage::Ready;
    state_.lastErrorCode = 0;
  }

  publishStatus(nowMs);
}

bool OTAService::validatePackage(unsigned long nowMs) {
  state_.stage = OtaStage::Validating;
  publishStatus(nowMs);

  if (!state_.updateAvailable || state_.targetVersion <= state_.currentVersion) {
    failApply(kErrIntegrity, nowMs, false);
    return false;
  }

  if (!state_.integrityOk) {
    failApply(kErrIntegrity, nowMs, false);
    return false;
  }

  return true;
}

bool OTAService::runPrecheck(unsigned long nowMs) {
  state_.stage = OtaStage::Precheck;
  state_.policyAllowed = policyAllowsApply();
  publishStatus(nowMs);

  if (!state_.policyAllowed) {
    failApply(kErrPolicyBlocked, nowMs, false);
    return false;
  }

  if (FeatureFlags::ROBUST_RECOVERY_ENABLED && rollbackMarker_.pending) {
    failApply(kErrRollbackPending, nowMs, true);
    return false;
  }

  return true;
}

void OTAService::startApply(unsigned long nowMs) {
  state_.safeModeForced = true;

  Event safeModeEvent;
  safeModeEvent.type = EventType::EVT_SAFE_MODE_CHANGED;
  safeModeEvent.source = EventSource::OTAService;
  safeModeEvent.value = 1;
  safeModeEvent.timestamp = nowMs;
  eventBus_.publish(safeModeEvent);

  applyStartMs_ = nowMs;
  applyAttemptCount_++;

  if (FeatureFlags::ROBUST_RECOVERY_ENABLED) {
    if (!createRollbackMarker(nowMs)) {
      failApply(kErrRollbackMarker, nowMs, false);
      return;
    }

    state_.stage = OtaStage::Writing;
  } else {
    state_.stage = OtaStage::Applying;
  }

  publishStatus(nowMs);
  diagnostics_.logInfo("[OTA] apply started");
}

void OTAService::finishApply(unsigned long nowMs) {
  if (FeatureFlags::ROBUST_RECOVERY_ENABLED) {
    state_.stage = OtaStage::Verifying;
    publishStatus(nowMs);

    if (state_.packageChecksum != makeIntegrityToken(state_.targetVersion)) {
      failApply(kErrVerifyFailed, nowMs, true);
      return;
    }
  }

  state_.currentVersion = state_.targetVersion;
  state_.updateAvailable = false;
  state_.stage = OtaStage::Applied;
  state_.lastErrorCode = 0;

  storageManager_.clearStagedOtaPackage();
  clearRollbackMarker();

  if (state_.safeModeForced) {
    Event safeModeEvent;
    safeModeEvent.type = EventType::EVT_SAFE_MODE_CHANGED;
    safeModeEvent.source = EventSource::OTAService;
    safeModeEvent.value = 0;
    safeModeEvent.timestamp = nowMs;
    eventBus_.publish(safeModeEvent);
    state_.safeModeForced = false;
  }

  publishStatus(nowMs);
  diagnostics_.logInfo("[OTA] apply complete");
}

void OTAService::failApply(int errorCode, unsigned long nowMs, bool keepRollbackPending) {
  state_.stage = OtaStage::Failed;
  state_.lastErrorCode = errorCode;

  if (FeatureFlags::ROBUST_RECOVERY_ENABLED) {
    if (rollbackMarker_.pending) {
      rollbackMarker_.lastErrorCode = static_cast<uint16_t>(errorCode);
      rollbackMarker_.attemptCount = applyAttemptCount_;
      if (!keepRollbackPending) {
        rollbackMarker_.pending = false;
      }
      storageManager_.saveRollbackMarker(rollbackMarker_);
    }

    persistFailureFault(static_cast<uint16_t>(errorCode), FaultSubsystem::Ota, nowMs);
  }

  if (state_.safeModeForced) {
    Event safeModeEvent;
    safeModeEvent.type = EventType::EVT_SAFE_MODE_CHANGED;
    safeModeEvent.source = EventSource::OTAService;
    safeModeEvent.value = 0;
    safeModeEvent.timestamp = nowMs;
    eventBus_.publish(safeModeEvent);
    state_.safeModeForced = false;
  }

  publishStatus(nowMs);
  diagnostics_.logError("[OTA] apply failed");
}

void OTAService::publishStatus(unsigned long nowMs) {
  Event event;
  event.type = EventType::EVT_OTA_STATUS;
  event.source = EventSource::OTAService;
  event.value = static_cast<int>(state_.stage);
  event.timestamp = nowMs;
  eventBus_.publish(event);

  lastStatusPublishMs_ = nowMs;
}

uint32_t OTAService::makeIntegrityToken(uint32_t version) const {
  return version ^ 0xA5A55A5AU;
}

void OTAService::handleRollbackMarkerAtBoot(unsigned long nowMs) {
  RollbackMarker marker{};
  if (!storageManager_.loadRollbackMarker(marker)) {
    rollbackMarker_ = RollbackMarker{};
    return;
  }

  rollbackMarker_ = marker;
  if (!rollbackMarker_.pending) {
    return;
  }

  state_.stage = OtaStage::Rollback;
  state_.lastErrorCode = kErrRollbackPending;

  persistFailureFault(kErrRollbackPending,
                      static_cast<uint16_t>(FaultSubsystem::Ota | FaultSubsystem::Storage),
                      nowMs);

  rollbackMarker_.pending = false;
  storageManager_.saveRollbackMarker(rollbackMarker_);
  diagnostics_.logError("[OTA] rollback marker detected on boot");
}

bool OTAService::createRollbackMarker(unsigned long nowMs) {
  rollbackMarker_ = RollbackMarker{};
  rollbackMarker_.pending = true;
  rollbackMarker_.fromVersion = state_.currentVersion;
  rollbackMarker_.targetVersion = state_.targetVersion;
  rollbackMarker_.packageChecksum = state_.packageChecksum;
  rollbackMarker_.createdAtMs = nowMs;
  rollbackMarker_.attemptCount = applyAttemptCount_;
  rollbackMarker_.lastErrorCode = 0;

  return storageManager_.saveRollbackMarker(rollbackMarker_);
}

void OTAService::clearRollbackMarker() {
  rollbackMarker_ = RollbackMarker{};
  storageManager_.clearRollbackMarker();
}

void OTAService::persistFailureFault(uint16_t code, uint16_t subsystemMask, unsigned long nowMs) {
  FaultRecord record;
  record.timestampMs = nowMs;
  record.bootCount = storageManager_.bootCount();
  record.subsystemMask = subsystemMask;
  record.code = code;
  record.source = static_cast<uint8_t>(EventSource::OTAService);
  record.severity = static_cast<uint8_t>(FaultSeverity::Error);
  storageManager_.appendFaultRecord(record);
}
