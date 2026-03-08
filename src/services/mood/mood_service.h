#pragma once

#include "../../core/event_bus.h"
#include "../../interfaces/i_emotion_provider.h"
#include "../../interfaces/i_event_listener.h"
#include "../../models/mood_state.h"

class MoodService : public IEventListener {
public:
  MoodService(EventBus& eventBus, const IEmotionProvider& emotionProvider);

  void init();
  void update(unsigned long nowMs);
  void onEvent(const Event& event) override;

  const MoodState& getState() const;

private:
  void publishMood(unsigned long nowMs);

  EventBus& eventBus_;
  const IEmotionProvider& emotionProvider_;

  MoodState state_{};
  MoodState lastPublished_{};
  unsigned long lastUpdateMs_ = 0;
};
