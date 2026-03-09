#pragma once

#include "../../core/event_bus.h"
#include "../../interfaces/i_event_listener.h"
#include "../../interfaces/i_persona_provider.h"
#include "../../models/persona_profile.h"

class PersonaService : public IEventListener, public IPersonaProvider {
public:
  explicit PersonaService(EventBus& eventBus);

  void init();
  void update(unsigned long nowMs);
  void onEvent(const Event& event) override;

  const PersonaProfile& getProfile() const override;

private:
  void publish(unsigned long nowMs);

  EventBus& eventBus_;
  PersonaProfile profile_{};
  PersonaProfile lastPublished_{};

  float mood_ = 0.0f;
  float engagement_ = 0.0f;
  float affinity_ = 0.25f;
  float memorySignal_ = 0.0f;
  float voicePreference_ = 0.0f;
  bool prefersCalm_ = false;

  unsigned long lastInteractionMs_ = 0;
  unsigned long lastUpdateMs_ = 0;
};
