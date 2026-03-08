#pragma once

#include "../../core/event_bus.h"
#include "../../interfaces/i_event_listener.h"
#include "../../models/cloud_types.h"
#include "../../models/power_state.h"

class CloudRouter : public IEventListener {
public:
  explicit CloudRouter(EventBus& eventBus);

  void init();
  void update(unsigned long nowMs);
  void onEvent(const Event& event) override;

private:
  struct PendingRequest {
    bool active = false;
    CloudRequestType type = CloudRequestType::None;
    unsigned long createdAtMs = 0;
    unsigned long startedAtMs = 0;
    unsigned long dueAtMs = 0;
    uint8_t retries = 0;
  };

  bool shouldUseCloud() const;
  bool startRequest(CloudRequestType type, unsigned long nowMs);
  void handlePending(unsigned long nowMs);
  void completeSuccess(unsigned long nowMs);
  void completeFailure(CloudResultCode reason, unsigned long nowMs);
  bool shouldSucceed(const PendingRequest& req) const;

  void publishCloudEvent(EventType type, CloudResultCode result, unsigned long nowMs);

  EventBus& eventBus_;
  PendingRequest pending_{};
  PowerMode powerMode_ = PowerMode::Normal;
  unsigned long lastCloudActivityMs_ = 0;
};
