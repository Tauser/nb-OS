#pragma once

#include "../../core/event_bus.h"
#include "../../interfaces/i_emotion_provider.h"
#include "../../interfaces/i_event_listener.h"
#include "../../models/emotion_state.h"
#include "../../models/homeostasis_state.h"

class EmotionService : public IEventListener, public IEmotionProvider {
public:
  explicit EmotionService(EventBus& eventBus);

  void init();
  void update(unsigned long nowMs);
  void onEvent(const Event& event) override;

  const EmotionState& getEmotionState() const override;
  const HomeostasisState& getHomeostasisState() const;
  void reset();

private:
  void applyEventRule(const Event& event);
  void applyHomeostaticDecay(float dtS);
  void applyAdaptiveDecay(float dtS);
  void computeEmotionalScores(unsigned long nowMs);
  void synthesizeEmotionFromHomeostasis();
  HomeostasisMode dominantModeWithHysteresis(unsigned long nowMs) const;

  void applyHomeoDelta(float dEnergy, float dStimulation, float dCuriosity);
  bool shouldPublish(unsigned long nowMs) const;
  void publishEmotionChanged(unsigned long nowMs);
  float stateDelta(const EmotionState& a, const EmotionState& b) const;

  EmotionState baseline_ = EmotionState::neutral();
  EmotionState state_ = EmotionState::neutral();
  EmotionState lastPublishedState_ = EmotionState::neutral();

  HomeostasisState homeo_{};
  HomeostasisState lastPublishedHomeo_{};

  EventBus& eventBus_;
  EventType lastObservedEventType_ = EventType::None;
  unsigned long lastUpdateMs_ = 0;
  unsigned long lastPublishMs_ = 0;
  unsigned long lastInteractionMs_ = 0;
  unsigned long modeSinceMs_ = 0;
  bool changedSincePublish_ = false;
};
