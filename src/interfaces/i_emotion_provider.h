#pragma once

#include "../models/emotion_state.h"

class IEmotionProvider {
public:
  virtual ~IEmotionProvider() = default;
  virtual const EmotionState& getEmotionState() const = 0;
};
