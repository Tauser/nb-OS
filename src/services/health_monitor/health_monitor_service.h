#pragma once

#include "../../core/diagnostics.h"
#include "../../core/event_bus.h"
#include "../../interfaces/i_event_listener.h"
#include "../../models/health_snapshot.h"

class HealthMonitorService : public IEventListener {
public:
  HealthMonitorService(EventBus& eventBus, Diagnostics& diagnostics);

  void init();
  void update(unsigned long nowMs);
  void onEvent(const Event& event) override;

  const HealthSnapshot& getSnapshot() const;

private:
  void updateWindowMetrics(unsigned long nowMs);
  void evaluateHealth(unsigned long nowMs);
  void publishStatus(unsigned long nowMs);
  void publishAnomaly(unsigned long nowMs, HealthAnomalyCode anomaly);

  EventBus& eventBus_;
  Diagnostics& diagnostics_;
  HealthSnapshot snapshot_{};

  unsigned long lastHeartbeatMs_ = 0;
  unsigned long lastWindowMs_ = 0;
  unsigned long lastStatusPublishMs_ = 0;
  unsigned long lastAnomalyPublishMs_ = 0;

  uint16_t eventsInWindow_ = 0;
  uint16_t renderedFramesInWindow_ = 0;
  HealthAnomalyCode lastAnomaly_ = HealthAnomalyCode::None;
};
