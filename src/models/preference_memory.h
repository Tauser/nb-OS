#pragma once

#include "attention_focus.h"

struct PreferenceMemory {
  AttentionFocus preferredFocus = AttentionFocus::Idle;
  bool prefersCalmStyle = false;
  unsigned long touchInteractions = 0;
  unsigned long voiceInteractions = 0;
  unsigned long visionInteractions = 0;
};
