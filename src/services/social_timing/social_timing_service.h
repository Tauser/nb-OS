#pragma once

#include "../../core/event_bus.h"
#include "../../interfaces/i_event_listener.h"
#include "../../interfaces/i_persona_provider.h"
#include "../../interfaces/i_social_timing_provider.h"
#include "../../models/attention_focus.h"
#include "../../models/social_timing_state.h"

class SocialTimingService : public IEventListener, public ISocialTimingProvider {
public:
  SocialTimingService(EventBus& eventBus, const IPersonaProvider& personaProvider);

  void init();
  void update(unsigned long nowMs);
  void onEvent(const Event& event) override;

  const SocialTimingState& getState() const override;

private:
  void publish(unsigned long nowMs);

  EventBus& eventBus_;
  const IPersonaProvider& personaProvider_;

  SocialTimingState state_{};
  SocialTimingState lastPublished_{};

  AttentionFocus focus_ = AttentionFocus::Idle;
  float engagement_ = 0.0f;
  float mood_ = 0.0f;
  float memorySignal_ = 0.0f;
  unsigned long lastInteractionMs_ = 0;
  unsigned long lastUpdateMs_ = 0;
};
