#pragma once

#include "../../core/event_bus.h"
#include "../../interfaces/i_event_listener.h"
#include "../../models/cloud_types.h"
#include "../../models/power_state.h"
#include "cloud_policy.h"
#include "cloud_transport.h"

class CloudRouter : public IEventListener {
public:
  explicit CloudRouter(EventBus& eventBus);

  void init();
  void update(unsigned long nowMs);
  void onEvent(const Event& event) override;

private:
  struct PendingRequest {
    bool active = false;
    bool waitingRetry = false;
    CloudRequestType type = CloudRequestType::None;
    unsigned long createdAtMs = 0;
    unsigned long startedAtMs = 0;
    unsigned long deadlineAtMs = 0;
    unsigned long retryAtMs = 0;
    uint8_t retries = 0;
  };

  bool shouldUseCloud() const;
  bool startRequest(CloudRequestType type, unsigned long nowMs);
  bool beginAttempt(unsigned long nowMs);
  void scheduleRetry(unsigned long nowMs);

  void handlePendingReal(unsigned long nowMs);
  void handlePendingLegacy(unsigned long nowMs);
  bool shouldSucceedLegacy(const PendingRequest& req) const;

  void completeSuccess(unsigned long nowMs);
  void completeFailure(CloudResultCode reason, unsigned long nowMs);
  void resetPending();

  void publishCloudEvent(EventType type, CloudResultCode result, unsigned long nowMs);

  EventBus& eventBus_;
  PendingRequest pending_{};
  PowerMode powerMode_ = PowerMode::Normal;
  unsigned long lastCloudActivityMs_ = 0;

  CloudTransport transport_{};
  CloudPolicy policy_{};
};
