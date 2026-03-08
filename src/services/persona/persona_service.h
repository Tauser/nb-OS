#pragma once

#include "../../core/event_bus.h"
#include "../../interfaces/i_event_listener.h"
#include "../../models/persona_profile.h"

class PersonaService : public IEventListener {
public:
  explicit PersonaService(EventBus& eventBus);

  void init();
  void update(unsigned long nowMs);
  void onEvent(const Event& event) override;

  const PersonaProfile& getProfile() const;

private:
  void publish(unsigned long nowMs);

  EventBus& eventBus_;
  PersonaProfile profile_{};
  PersonaProfile lastPublished_{};
  float mood_ = 0.0f;
  float engagement_ = 0.0f;
  float affinity_ = 0.25f;
  bool prefersCalm_ = false;
  unsigned long lastUpdateMs_ = 0;
};
