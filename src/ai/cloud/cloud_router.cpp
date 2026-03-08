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
}

void CloudRouter::update(unsigned long nowMs) {
  handlePending(nowMs);
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

  if (!shouldUseCloud()) {
    publishCloudEvent(EventType::EVT_CLOUD_FALLBACK, CloudResultCode::SkippedOffline, nowMs);
    return;
  }

  if (pending_.active) {
    return;
  }

  startRequest(requestType, nowMs);
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

  if (nowMs - lastCloudActivityMs_ < HardwareConfig::Cloud::REQUEST_COOLDOWN_MS) {
    return false;
  }

  pending_.active = true;
  pending_.type = type;
  pending_.createdAtMs = nowMs;
  pending_.startedAtMs = nowMs;
  pending_.dueAtMs = nowMs + HardwareConfig::Cloud::REMOTE_LATENCY_MS;
  pending_.retries = 0;
  return true;
}

void CloudRouter::handlePending(unsigned long nowMs) {
  if (!pending_.active) {
    return;
  }

  if (nowMs - pending_.startedAtMs >= HardwareConfig::Cloud::REQUEST_TIMEOUT_MS) {
    if (pending_.retries < HardwareConfig::Cloud::MAX_RETRIES) {
      pending_.retries++;
      pending_.startedAtMs = nowMs;
      pending_.dueAtMs = nowMs + HardwareConfig::Cloud::REMOTE_LATENCY_MS;
      return;
    }

    completeFailure(CloudResultCode::Timeout, nowMs);
    return;
  }

  if (nowMs < pending_.dueAtMs) {
    return;
  }

  if (shouldSucceed(pending_)) {
    completeSuccess(nowMs);
  } else {
    if (pending_.retries < HardwareConfig::Cloud::MAX_RETRIES) {
      pending_.retries++;
      pending_.startedAtMs = nowMs;
      pending_.dueAtMs = nowMs + HardwareConfig::Cloud::REMOTE_LATENCY_MS;
      return;
    }

    completeFailure(CloudResultCode::Failed, nowMs);
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
  pending_ = PendingRequest{};
}

void CloudRouter::completeFailure(CloudResultCode reason, unsigned long nowMs) {
  publishCloudEvent(EventType::EVT_CLOUD_FALLBACK, reason, nowMs);
  lastCloudActivityMs_ = nowMs;
  pending_ = PendingRequest{};
}

bool CloudRouter::shouldSucceed(const PendingRequest& req) const {
  // Deterministic success policy placeholder; replace with real transport result.
  switch (req.type) {
    case CloudRequestType::VoiceUnknownIntent:
      return true;
    case CloudRequestType::VisionMotion:
      return (req.retries > 0);
    case CloudRequestType::VisionDark:
      return false;
    default:
      return false;
  }
}

void CloudRouter::publishCloudEvent(EventType type, CloudResultCode result, unsigned long nowMs) {
  Event event;
  event.type = type;
  event.source = EventSource::CloudRouter;
  event.value = static_cast<int>(result);
  event.timestamp = nowMs;
  eventBus_.publish(event);
}
