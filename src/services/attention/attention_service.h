#pragma once

#include "../../core/event_bus.h"
#include "../../interfaces/i_event_listener.h"
#include "../../models/attention_focus.h"

class AttentionService : public IEventListener {
public:
  explicit AttentionService(EventBus& eventBus);

  void init();
  void update(unsigned long nowMs);
  void onEvent(const Event& event) override;

  AttentionFocus getFocus() const;

private:
  void setFocus(AttentionFocus focus, unsigned long nowMs, unsigned long holdMs);
  void publishFocus(AttentionFocus focus, unsigned long nowMs);

  EventBus& eventBus_;
  AttentionFocus focus_ = AttentionFocus::Idle;
  unsigned long focusUntilMs_ = 0;
  unsigned long lastInteractionMs_ = 0;
  unsigned long lastInternalPulseMs_ = 0;
  unsigned long lastScanAttemptMs_ = 0;
};
