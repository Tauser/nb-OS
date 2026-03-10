#include "cloud_transport.h"

#include "../../config/hardware_config.h"
#include <Arduino.h>

void CloudTransport::init() {
  busy_ = false;
  request_ = CloudTransportRequest{};
  plannedResult_ = CloudResultCode::None;
  startMs_ = 0;
  completeAtMs_ = 0;
}

bool CloudTransport::begin(const CloudTransportRequest& request, unsigned long nowMs) {
  if (busy_ || request.type == CloudRequestType::None) {
    return false;
  }

  request_ = request;
  plannedResult_ = evaluateOutcome(request);
  startMs_ = nowMs;

  const unsigned long jitterMs = static_cast<unsigned long>(esp_random() % 120U);
  completeAtMs_ = nowMs + HardwareConfig::Cloud::REMOTE_LATENCY_MS + jitterMs;
  busy_ = true;
  return true;
}

bool CloudTransport::poll(unsigned long nowMs, CloudTransportResponse& outResponse) {
  if (!busy_) {
    return false;
  }

  if (nowMs < completeAtMs_) {
    return false;
  }

  outResponse.result = plannedResult_;
  outResponse.success = (plannedResult_ == CloudResultCode::Success);
  outResponse.latencyMs = nowMs - startMs_;

  busy_ = false;
  request_ = CloudTransportRequest{};
  plannedResult_ = CloudResultCode::None;
  startMs_ = 0;
  completeAtMs_ = 0;
  return true;
}

void CloudTransport::cancel() {
  busy_ = false;
  request_ = CloudTransportRequest{};
  plannedResult_ = CloudResultCode::None;
  startMs_ = 0;
  completeAtMs_ = 0;
}

bool CloudTransport::busy() const {
  return busy_;
}

CloudResultCode CloudTransport::evaluateOutcome(const CloudTransportRequest& request) const {
  if (HardwareConfig::Cloud::AUTH_REQUIRED) {
    if (request.authToken == nullptr || request.authToken[0] == '\0') {
      return CloudResultCode::AuthFailed;
    }
  }

  switch (request.type) {
    case CloudRequestType::VoiceUnknownIntent:
      return CloudResultCode::Success;
    case CloudRequestType::VisionMotion:
      return (esp_random() % 100U < 75U) ? CloudResultCode::Success : CloudResultCode::Failed;
    case CloudRequestType::VisionDark:
      return (esp_random() % 100U < 55U) ? CloudResultCode::Success : CloudResultCode::Failed;
    case CloudRequestType::None:
    default:
      return CloudResultCode::Failed;
  }
}
