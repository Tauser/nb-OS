#include "cloud_policy.h"

#include "../../config/hardware_config.h"

void CloudPolicy::init() {
  consecutiveFailures_ = 0;
  circuitOpenUntilMs_ = 0;
}

bool CloudPolicy::canStart(bool cloudEnabled, PowerMode powerMode, unsigned long nowMs, CloudResultCode& outReason) const {
  if (!cloudEnabled) {
    outReason = CloudResultCode::SkippedOffline;
    return false;
  }

  if (powerMode != PowerMode::Normal && powerMode != PowerMode::Charging) {
    outReason = CloudResultCode::SkippedOffline;
    return false;
  }

  if (isCircuitOpen(nowMs)) {
    outReason = CloudResultCode::CircuitOpen;
    return false;
  }

  outReason = CloudResultCode::None;
  return true;
}

unsigned long CloudPolicy::timeoutFor(CloudRequestType type) const {
  switch (type) {
    case CloudRequestType::VoiceUnknownIntent:
      return HardwareConfig::Cloud::REQUEST_TIMEOUT_VOICE_MS;
    case CloudRequestType::VisionMotion:
    case CloudRequestType::VisionDark:
      return HardwareConfig::Cloud::REQUEST_TIMEOUT_VISION_MS;
    case CloudRequestType::None:
    default:
      return HardwareConfig::Cloud::REQUEST_TIMEOUT_MS;
  }
}

bool CloudPolicy::shouldRetry(CloudResultCode failure, uint8_t attempt) const {
  if (attempt >= HardwareConfig::Cloud::MAX_RETRIES) {
    return false;
  }

  switch (failure) {
    case CloudResultCode::Timeout:
    case CloudResultCode::Failed:
      return true;
    case CloudResultCode::AuthFailed:
    case CloudResultCode::CircuitOpen:
    case CloudResultCode::SkippedOffline:
    case CloudResultCode::RateLimited:
    case CloudResultCode::Success:
    case CloudResultCode::None:
    default:
      return false;
  }
}

unsigned long CloudPolicy::retryDelayMs(uint8_t attempt) const {
  return HardwareConfig::Cloud::RETRY_BACKOFF_BASE_MS +
         (HardwareConfig::Cloud::RETRY_BACKOFF_STEP_MS * static_cast<unsigned long>(attempt));
}

void CloudPolicy::onAttemptFailure(CloudResultCode failure, unsigned long nowMs) {
  switch (failure) {
    case CloudResultCode::Timeout:
    case CloudResultCode::Failed:
    case CloudResultCode::AuthFailed:
      consecutiveFailures_++;
      break;
    default:
      return;
  }

  if (consecutiveFailures_ >= HardwareConfig::Cloud::CIRCUIT_BREAKER_FAIL_THRESHOLD) {
    circuitOpenUntilMs_ = nowMs + HardwareConfig::Cloud::CIRCUIT_BREAKER_OPEN_MS;
    consecutiveFailures_ = 0;
  }
}

void CloudPolicy::onFinalResult(CloudResultCode result, unsigned long nowMs) {
  if (result == CloudResultCode::Success) {
    consecutiveFailures_ = 0;
    circuitOpenUntilMs_ = 0;
    return;
  }

  onAttemptFailure(result, nowMs);
}

bool CloudPolicy::isCircuitOpen(unsigned long nowMs) const {
  return nowMs < circuitOpenUntilMs_;
}
