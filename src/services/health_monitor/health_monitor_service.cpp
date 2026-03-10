#include "health_monitor_service.h"

#include "../../config/feature_flags.h"
#include "../../config/hardware_config.h"
#include "../../models/event.h"
#include <Arduino.h>

#ifndef NCOS_SIM_MODE
#define NCOS_SIM_MODE 0
#endif

HealthMonitorService::HealthMonitorService(EventBus& eventBus, Diagnostics& diagnostics)
    : eventBus_(eventBus), diagnostics_(diagnostics) {}

void HealthMonitorService::init() {
  eventBus_.subscribe(EventType::Any, this);

  const unsigned long nowMs = millis();
  lastHeartbeatMs_ = nowMs;
  lastActivityMs_ = nowMs;
  lastWindowMs_ = nowMs;
  lastStatusPublishMs_ = nowMs;
  lastAnomalyPublishMs_ = nowMs;

  snapshot_.freeHeapBytes = ESP.getFreeHeap();
  snapshot_.minFreeHeapBytes = ESP.getMinFreeHeap();
}

void HealthMonitorService::update(unsigned long nowMs) {
  if (nowMs - lastWindowMs_ >= HardwareConfig::Health::METRIC_WINDOW_MS) {
    updateWindowMetrics(nowMs);
  }

  evaluateHealth(nowMs);

  if (nowMs - lastStatusPublishMs_ >= HardwareConfig::Health::STATUS_PUBLISH_INTERVAL_MS) {
    publishStatus(nowMs);
    lastStatusPublishMs_ = nowMs;
  }

  if (snapshot_.primaryAnomaly != lastAnomaly_ && snapshot_.primaryAnomaly != HealthAnomalyCode::None) {
    if (nowMs - lastAnomalyPublishMs_ >= HardwareConfig::Health::ANOMALY_PUBLISH_COOLDOWN_MS) {
      publishAnomaly(nowMs, snapshot_.primaryAnomaly);
      lastAnomalyPublishMs_ = nowMs;
    }
  }

  lastAnomaly_ = snapshot_.primaryAnomaly;
}

void HealthMonitorService::onEvent(const Event& event) {
  if (event.source == EventSource::HealthMonitorService ||
      event.type == EventType::EVT_HEALTH_STATUS ||
      event.type == EventType::EVT_HEALTH_ANOMALY) {
    return;
  }

  eventsInWindow_++;
  lastActivityMs_ = event.timestamp;

  if (event.type == EventType::Heartbeat) {
    lastHeartbeatMs_ = event.timestamp;
  }

  if (event.type == EventType::FaceFrameRendered) {
    renderedFramesInWindow_++;
  }
}

const HealthSnapshot& HealthMonitorService::getSnapshot() const {
  return snapshot_;
}

void HealthMonitorService::updateWindowMetrics(unsigned long nowMs) {
  const unsigned long elapsedMs = nowMs - lastWindowMs_;
  if (elapsedMs == 0UL) {
    return;
  }

  snapshot_.eventRatePerSecond = static_cast<uint16_t>((static_cast<uint32_t>(eventsInWindow_) * 1000UL) / elapsedMs);
  snapshot_.renderFps = static_cast<uint16_t>((static_cast<uint32_t>(renderedFramesInWindow_) * 1000UL) / elapsedMs);

  eventsInWindow_ = 0;
  renderedFramesInWindow_ = 0;
  lastWindowMs_ = nowMs;
}

void HealthMonitorService::evaluateHealth(unsigned long nowMs) {
  snapshot_.freeHeapBytes = ESP.getFreeHeap();
  snapshot_.minFreeHeapBytes = ESP.getMinFreeHeap();

#if NCOS_SIM_MODE
  // In simulator, scheduler jitter and virtual timing can make heartbeat unreliable.
  snapshot_.heartbeatOk = true;
#else
  const bool heartbeatFresh = (nowMs - lastHeartbeatMs_) <= HardwareConfig::Health::HEARTBEAT_TIMEOUT_MS;
  const bool activityFresh = (nowMs - lastActivityMs_) <= HardwareConfig::Health::HEARTBEAT_TIMEOUT_MS;
  snapshot_.heartbeatOk = heartbeatFresh || activityFresh;
#endif
  snapshot_.renderOk = !FeatureFlags::DISPLAY_ENABLED || (snapshot_.renderFps >= HardwareConfig::Health::MIN_RENDER_FPS);
  snapshot_.memoryOk = snapshot_.freeHeapBytes >= HardwareConfig::Health::MIN_FREE_HEAP_BYTES;
  snapshot_.eventLoadOk = snapshot_.eventRatePerSecond <= HardwareConfig::Health::MAX_EVENT_RATE_PER_S;

  int score = 100;
  if (!snapshot_.heartbeatOk) score -= 35;
  if (!snapshot_.renderOk) score -= 25;
  if (!snapshot_.memoryOk) score -= 25;
  if (!snapshot_.eventLoadOk) score -= 15;
  if (score < 0) score = 0;

  snapshot_.healthScore = static_cast<uint8_t>(score);
  snapshot_.degraded = (score < 100);

  if (!snapshot_.heartbeatOk) {
    snapshot_.primaryAnomaly = HealthAnomalyCode::HeartbeatTimeout;
  } else if (!snapshot_.memoryOk) {
    snapshot_.primaryAnomaly = HealthAnomalyCode::LowMemory;
  } else if (!snapshot_.renderOk) {
    snapshot_.primaryAnomaly = HealthAnomalyCode::LowRenderFps;
  } else if (!snapshot_.eventLoadOk) {
    snapshot_.primaryAnomaly = HealthAnomalyCode::HighEventLoad;
  } else {
    snapshot_.primaryAnomaly = HealthAnomalyCode::None;
  }
}

void HealthMonitorService::publishStatus(unsigned long nowMs) {
  Event status;
  status.type = EventType::EVT_HEALTH_STATUS;
  status.source = EventSource::HealthMonitorService;

  HealthStatusPayload payload{};
  payload.healthScore = static_cast<int>(snapshot_.healthScore);
  payload.primaryAnomaly = static_cast<int>(snapshot_.primaryAnomaly);
  payload.degraded = snapshot_.degraded;
  payload.heartbeatOk = snapshot_.heartbeatOk;
  payload.renderOk = snapshot_.renderOk;
  payload.memoryOk = snapshot_.memoryOk;
  payload.eventLoadOk = snapshot_.eventLoadOk;
  status.setHealthStatus(payload);

  status.timestamp = nowMs;
  eventBus_.publish(status);
}

void HealthMonitorService::publishAnomaly(unsigned long nowMs, HealthAnomalyCode anomaly) {
  Event event;
  event.type = EventType::EVT_HEALTH_ANOMALY;
  event.source = EventSource::HealthMonitorService;
  event.value = static_cast<int>(anomaly);
  event.timestamp = nowMs;
  eventBus_.publish(event);

  switch (anomaly) {
    case HealthAnomalyCode::HeartbeatTimeout:
      diagnostics_.logError("[HEALTH] heartbeat timeout");
      break;
    case HealthAnomalyCode::LowRenderFps:
      diagnostics_.logError("[HEALTH] render fps degraded");
      break;
    case HealthAnomalyCode::LowMemory:
      diagnostics_.logError("[HEALTH] low free heap");
      break;
    case HealthAnomalyCode::HighEventLoad:
      diagnostics_.logError("[HEALTH] event rate high");
      break;
    default:
      break;
  }
}


