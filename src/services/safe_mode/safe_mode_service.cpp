#include "safe_mode_service.h"

#include "../../models/event.h"
#include "../../models/health_snapshot.h"
#include <Arduino.h>

SafeModeService::SafeModeService(EventBus& eventBus, Diagnostics& diagnostics, IMotion& motion)
    : eventBus_(eventBus), diagnostics_(diagnostics), motion_(motion) {}

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
      enterSafeMode(event.timestamp, "self-test failure");
    }
    return;
  }

  if (event.type == EventType::EVT_HEALTH_ANOMALY) {
    const HealthAnomalyCode anomaly = static_cast<HealthAnomalyCode>(event.value);
    if (anomaly == HealthAnomalyCode::HeartbeatTimeout ||
        anomaly == HealthAnomalyCode::LowMemory) {
      enterSafeMode(event.timestamp, "runtime health anomaly");
    }
  }
}

bool SafeModeService::isActive() const {
  return active_;
}

void SafeModeService::enterSafeMode(unsigned long nowMs, const char* reason) {
  if (active_) {
    return;
  }

  active_ = true;
  motion_.center();

  Event event;
  event.type = EventType::EVT_SAFE_MODE_CHANGED;
  event.source = EventSource::SafeModeService;
  event.value = 1;
  event.timestamp = nowMs;
  eventBus_.publish(event);

  diagnostics_.logError("[SAFE_MODE] enabled");
  diagnostics_.logError(reason);
}
