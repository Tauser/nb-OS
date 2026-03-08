#pragma once

#include "../../core/event_bus.h"
#include "../../interfaces/i_event_listener.h"
#include "../../models/preference_memory.h"

class PreferenceMemoryService : public IEventListener {
public:
  explicit PreferenceMemoryService(EventBus& eventBus);

  void init();
  void update(unsigned long nowMs);
  void onEvent(const Event& event) override;

  const PreferenceMemory& getMemory() const;

private:
  void recalcPreferences();
  void publish(unsigned long nowMs);

  EventBus& eventBus_;
  PreferenceMemory memory_{};
  PreferenceMemory lastPublished_{};
  unsigned long lastUpdateMs_ = 0;
};
