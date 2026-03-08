#pragma once

#include "../../core/event_bus.h"
#include "../../interfaces/i_emotion_provider.h"
#include "../../interfaces/i_event_listener.h"
#include "../../models/mood_state.h"
#include "../../models/power_state.h"
#include "../../models/routine_state.h"

class MoodService : public IEventListener {
public:
  MoodService(EventBus& eventBus, const IEmotionProvider& emotionProvider);

  void init();
  void update(unsigned long nowMs);
  void onEvent(const Event& event) override;

  const MoodState& getState() const;

private:
  void publishMood(unsigned long nowMs);
  void publishMoodProfile(unsigned long nowMs);
  void markInteraction(unsigned long nowMs);
  MoodProfile classifyProfile() const;

  EventBus& eventBus_;
  const IEmotionProvider& emotionProvider_;

  MoodState state_{};
  MoodState lastPublished_{};
  MoodProfile lastPublishedProfile_ = MoodProfile::Calm;

  unsigned long lastUpdateMs_ = 0;
  unsigned long lastInteractionMs_ = 0;
  PowerMode powerMode_ = PowerMode::Normal;
  RoutineState routineState_ = RoutineState::Idle;
  float memorySignal_ = 0.0f;
};
