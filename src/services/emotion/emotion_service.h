#pragma once

#include "../../core/event_bus.h"
#include "../../interfaces/i_emotion_provider.h"
#include "../../interfaces/i_event_listener.h"
#include "../../models/emotion_state.h"

class EmotionService : public IEventListener, public IEmotionProvider {
public:
  explicit EmotionService(EventBus& eventBus);

  void init();
  void update(unsigned long nowMs);
  void onEvent(const Event& event) override;

  const EmotionState& getEmotionState() const override;
  void reset();

private:
  void applyEventRule(const Event& event);
  void applyDecay(float dtS);
  void applyDelta(float dValence,
                  float dArousal,
                  float dCuriosity,
                  float dAttention,
                  float dBond,
                  float dEnergy);
  bool shouldPublish(unsigned long nowMs) const;
  void publishEmotionChanged(unsigned long nowMs);
  float stateDelta(const EmotionState& a, const EmotionState& b) const;

  EmotionState baseline_ = EmotionState::neutral();
  EmotionState state_ = EmotionState::neutral();
  EmotionState lastPublishedState_ = EmotionState::neutral();

  EventBus& eventBus_;
  EventType lastObservedEventType_ = EventType::None;
  unsigned long lastUpdateMs_ = 0;
  unsigned long lastPublishMs_ = 0;
  bool changedSincePublish_ = false;
};
