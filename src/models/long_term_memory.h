#pragma once

#include "attention_focus.h"
#include <stdint.h>

struct LongTermMemory {
  uint32_t schemaVersion = 2;
  char userName[24] = "";

  uint32_t totalSessions = 0;
  uint32_t totalInteractions = 0;
  uint32_t touchInteractions = 0;
  uint32_t voiceInteractions = 0;
  uint32_t visionInteractions = 0;
  uint32_t intentInteractions = 0;

  uint32_t cumulativeSessionDurationMs = 0;
  uint32_t avgInteractionsPerSessionX100 = 0;
  uint32_t boredomEpisodes = 0;
  uint16_t recurringIntentMask = 0;

  AttentionFocus preferredFocus = AttentionFocus::Idle;
  bool prefersCalmStyle = false;
  uint8_t calmStyleScore = 50;

  void sanitize() {
    if (schemaVersion == 0) {
      schemaVersion = 2;
    }

    userName[sizeof(userName) - 1] = '\0';

    if (preferredFocus == AttentionFocus::None) {
      preferredFocus = AttentionFocus::Idle;
    }

    if (calmStyleScore > 100) {
      calmStyleScore = 100;
    }
  }
};
