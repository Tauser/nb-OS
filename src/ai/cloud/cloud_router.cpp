#include "cloud_router.h"

#include "../../config/feature_flags.h"
#include "../../config/hardware_config.h"
#include "../../models/event.h"
#include "../../models/intent_types.h"

CloudRouter::CloudRouter(EventBus& eventBus)
    : eventBus_(eventBus) {}

void CloudRouter::init() {
  eventBus_.subscribe(EventType::EVT_CLOUD_REQUEST, this);
  eventBus_.subscribe(EventType::EVT_POWER_MODE_CHANGED, this);
  transport_.init();
  policy_.init();
}

void CloudRouter::update(unsigned long nowMs) {
  if (FeatureFlags::CLOUD_REAL_ENABLED) {
    handlePendingReal(nowMs);
  } else {
    handlePendingLegacy(nowMs);
  }
}

void CloudRouter::onEvent(const Event& event) {
  if (event.type == EventType::EVT_POWER_MODE_CHANGED) {
    powerMode_ = static_cast<PowerMode>(event.value);
    if (powerMode_ == PowerMode::Sleep && pending_.active) {
      completeFailure(CloudResultCode::SkippedOffline, event.timestamp);
    }
    return;
  }

  if (event.type != EventType::EVT_CLOUD_REQUEST) {
    return;
  }

  const CloudRequestType requestType = static_cast<CloudRequestType>(event.value);
  const unsigned long nowMs = event.timestamp;

  if (pending_.active) {
    return;
  }

  if (!startRequest(requestType, nowMs)) {
    return;
  }

  if (!FeatureFlags::CLOUD_REAL_ENABLED) {
    // Legacy path starts timing only; completion remains deterministic.
    return;
  }

  if (!beginAttempt(nowMs)) {
    completeFailure(CloudResultCode::Failed, nowMs);
  }
}

bool CloudRouter::shouldUseCloud() const {
  if (!FeatureFlags::CLOUD_ENABLED) {
    return false;
  }

  return (powerMode_ == PowerMode::Normal || powerMode_ == PowerMode::Charging);
}

bool CloudRouter::startRequest(CloudRequestType type, unsigned long nowMs) {
  if (type == CloudRequestType::None) {
    return false;
  }

  if (!FeatureFlags::CLOUD_REAL_ENABLED) {
    if (!shouldUseCloud()) {
      publishCloudEvent(EventType::EVT_CLOUD_FALLBACK, CloudResultCode::SkippedOffline, nowMs);
      return false;
    }

    if (nowMs - lastCloudActivityMs_ < HardwareConfig::Cloud::REQUEST_COOLDOWN_MS) {
      publishCloudEvent(EventType::EVT_CLOUD_FALLBACK, CloudResultCode::RateLimited, nowMs);
      return false;
    }

    pending_.active = true;
    pending_.waitingRetry = false;
    pending_.type = type;
    pending_.createdAtMs = nowMs;
    pending_.startedAtMs = nowMs;
    pending_.deadlineAtMs = nowMs + HardwareConfig::Cloud::REQUEST_TIMEOUT_MS;
    pending_.retryAtMs = 0;
    pending_.retries = 0;
    return true;
  }

  CloudResultCode rejectReason = CloudResultCode::None;
  if (!policy_.canStart(FeatureFlags::CLOUD_ENABLED, powerMode_, nowMs, rejectReason)) {
    publishCloudEvent(EventType::EVT_CLOUD_FALLBACK, rejectReason, nowMs);
    return false;
  }

  if (nowMs - lastCloudActivityMs_ < HardwareConfig::Cloud::REQUEST_COOLDOWN_MS) {
    publishCloudEvent(EventType::EVT_CLOUD_FALLBACK, CloudResultCode::RateLimited, nowMs);
    return false;
  }

  pending_.active = true;
  pending_.waitingRetry = false;
  pending_.type = type;
  pending_.createdAtMs = nowMs;
  pending_.startedAtMs = 0;
  pending_.deadlineAtMs = 0;
  pending_.retryAtMs = 0;
  pending_.retries = 0;
  return true;
}

bool CloudRouter::beginAttempt(unsigned long nowMs) {
  CloudTransportRequest request;
  request.type = pending_.type;
  request.timeoutMs = policy_.timeoutFor(pending_.type);
  request.authToken = HardwareConfig::Cloud::AUTH_TOKEN;

  if (!transport_.begin(request, nowMs)) {
    return false;
  }

  pending_.startedAtMs = nowMs;
  pending_.deadlineAtMs = nowMs + request.timeoutMs;
  pending_.waitingRetry = false;
  pending_.retryAtMs = 0;
  return true;
}

void CloudRouter::scheduleRetry(unsigned long nowMs) {
  pending_.retries++;
  pending_.waitingRetry = true;
  pending_.retryAtMs = nowMs + policy_.retryDelayMs(pending_.retries);
  pending_.startedAtMs = 0;
  pending_.deadlineAtMs = 0;
  transport_.cancel();
}

void CloudRouter::handlePendingReal(unsigned long nowMs) {
  if (!pending_.active) {
    return;
  }

  if (pending_.waitingRetry) {
    if (nowMs < pending_.retryAtMs) {
      return;
    }

    CloudResultCode rejectReason = CloudResultCode::None;
    if (!policy_.canStart(FeatureFlags::CLOUD_ENABLED, powerMode_, nowMs, rejectReason)) {
      completeFailure(rejectReason, nowMs);
      return;
    }

    if (!beginAttempt(nowMs)) {
      completeFailure(CloudResultCode::Failed, nowMs);
    }
    return;
  }

  if (pending_.startedAtMs == 0) {
    return;
  }

  if (nowMs >= pending_.deadlineAtMs) {
    policy_.onAttemptFailure(CloudResultCode::Timeout, nowMs);
    if (policy_.shouldRetry(CloudResultCode::Timeout, pending_.retries)) {
      scheduleRetry(nowMs);
      return;
    }

    policy_.onFinalResult(CloudResultCode::Timeout, nowMs);
    completeFailure(CloudResultCode::Timeout, nowMs);
    return;
  }

  CloudTransportResponse response;
  if (!transport_.poll(nowMs, response)) {
    return;
  }

  if (response.success) {
    policy_.onFinalResult(CloudResultCode::Success, nowMs);
    completeSuccess(nowMs);
    return;
  }

  policy_.onAttemptFailure(response.result, nowMs);
  if (policy_.shouldRetry(response.result, pending_.retries)) {
    scheduleRetry(nowMs);
    return;
  }

  policy_.onFinalResult(response.result, nowMs);
  completeFailure(response.result, nowMs);
}

void CloudRouter::handlePendingLegacy(unsigned long nowMs) {
  if (!pending_.active) {
    return;
  }

  if (nowMs >= pending_.deadlineAtMs) {
    if (pending_.retries < HardwareConfig::Cloud::MAX_RETRIES) {
      pending_.retries++;
      pending_.startedAtMs = nowMs;
      pending_.deadlineAtMs = nowMs + HardwareConfig::Cloud::REQUEST_TIMEOUT_MS;
      return;
    }

    completeFailure(CloudResultCode::Timeout, nowMs);
    return;
  }

  if (nowMs - pending_.startedAtMs < HardwareConfig::Cloud::REMOTE_LATENCY_MS) {
    return;
  }

  if (shouldSucceedLegacy(pending_)) {
    completeSuccess(nowMs);
  } else if (pending_.retries < HardwareConfig::Cloud::MAX_RETRIES) {
    pending_.retries++;
    pending_.startedAtMs = nowMs;
    pending_.deadlineAtMs = nowMs + HardwareConfig::Cloud::REQUEST_TIMEOUT_MS;
  } else {
    completeFailure(CloudResultCode::Failed, nowMs);
  }
}

bool CloudRouter::shouldSucceedLegacy(const PendingRequest& req) const {
  switch (req.type) {
    case CloudRequestType::VoiceUnknownIntent:
      return true;
    case CloudRequestType::VisionMotion:
      return (req.retries > 0);
    case CloudRequestType::VisionDark:
      return false;
    case CloudRequestType::None:
    default:
      return false;
  }
}

void CloudRouter::completeSuccess(unsigned long nowMs) {
  publishCloudEvent(EventType::EVT_CLOUD_RESPONSE, CloudResultCode::Success, nowMs);

  if (pending_.type == CloudRequestType::VoiceUnknownIntent) {
    Event event;
    event.type = EventType::EVT_INTENT_DETECTED;
    event.source = EventSource::CloudRouter;
    event.value = static_cast<int>(LocalIntent::Status);
    event.timestamp = nowMs;
    eventBus_.publish(event);
  }

  lastCloudActivityMs_ = nowMs;
  resetPending();
}

void CloudRouter::completeFailure(CloudResultCode reason, unsigned long nowMs) {
  publishCloudEvent(EventType::EVT_CLOUD_FALLBACK, reason, nowMs);
  lastCloudActivityMs_ = nowMs;
  resetPending();
}

void CloudRouter::resetPending() {
  transport_.cancel();
  pending_ = PendingRequest{};
}

void CloudRouter::publishCloudEvent(EventType type, CloudResultCode result, unsigned long nowMs) {
  Event event;
  event.type = type;
  event.source = EventSource::CloudRouter;
  event.value = static_cast<int>(result);
  event.timestamp = nowMs;
  eventBus_.publish(event);
}
