#pragma once

#include "../../models/intent_types.h"

class IntentEngine {
public:
  bool init();
  LocalIntent classify(const VoiceCommandFrame& frame) const;

private:
  LocalIntent classifySingleBurst(const VoiceCommandFrame& frame) const;
};
