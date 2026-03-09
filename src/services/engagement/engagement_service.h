#pragma once

#include "../../core/event_bus.h"
#include "../../interfaces/i_event_listener.h"
#include "../../models/attention_focus.h"
#include "../../models/engagement_state.h"

class EngagementService : public IEventListener {
public:
  explicit EngagementService(EventBus& eventBus);

  void init();
  void update(unsigned long nowMs);
  void onEvent(const Event& event) override;

  const EngagementState& getState() const;

private:
  void publish(unsigned long nowMs);

  EventBus& eventBus_;
  EngagementState state_{};
  float lastPublished_ = 0.0f;
  AttentionFocus focus_ = AttentionFocus::Idle;
  float memorySignal_ = 0.0f;
  unsigned long lastInteractionMs_ = 0;
  unsigned long lastUpdateMs_ = 0;
};
