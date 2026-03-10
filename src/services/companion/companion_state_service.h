#pragma once

#include "../../core/event_bus.h"
#include "../../interfaces/i_companion_state_provider.h"
#include "../../interfaces/i_event_listener.h"
#include "../../interfaces/i_social_timing_provider.h"

class CompanionStateService : public IEventListener, public ICompanionStateProvider {
public:
  CompanionStateService(EventBus& eventBus, const ISocialTimingProvider& socialTimingProvider);

  void init();
  void update(unsigned long nowMs);
  void onEvent(const Event& event) override;

  const CompanionState& getState() const override;

private:
  void publish(unsigned long nowMs);

  EventBus& eventBus_;
  const ISocialTimingProvider& socialTimingProvider_;

  CompanionState state_{};
  CompanionState lastPublished_{};
};
