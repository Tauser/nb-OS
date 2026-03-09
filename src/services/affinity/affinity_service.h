#pragma once

#include "../../core/event_bus.h"
#include "../../interfaces/i_event_listener.h"
#include "../../models/affinity_state.h"

class AffinityService : public IEventListener {
public:
  explicit AffinityService(EventBus& eventBus);

  void init();
  void update(unsigned long nowMs);
  void onEvent(const Event& event) override;

  const AffinityState& getState() const;

private:
  void publish(unsigned long nowMs);

  EventBus& eventBus_;
  AffinityState state_{};
  float lastPublished_ = 0.0f;
  float memorySignal_ = 0.0f;
  bool prefersVoice_ = false;
  unsigned long lastUpdateMs_ = 0;
};
