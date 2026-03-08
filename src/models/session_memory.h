#pragma once

#include "attention_focus.h"
#include <stdint.h>

struct SessionMemory {
  uint32_t sessionStartMs = 0;
  uint32_t lastInteractionMs = 0;
  uint32_t totalInteractions = 0;
  uint32_t touchInteractions = 0;
  uint32_t voiceInteractions = 0;
  uint32_t visionInteractions = 0;
  uint32_t intentInteractions = 0;
  uint32_t idleTicks = 0;
  uint32_t boredEpisodes = 0;
  uint32_t longestSilenceMs = 0;

  AttentionFocus dominantFocus = AttentionFocus::Idle;
  bool prefersCalmStyle = false;
  uint8_t dominantIntent = 0;

  void markInteraction(unsigned long nowMs) {
    if (lastInteractionMs > 0 && nowMs > lastInteractionMs) {
      const uint32_t silence = static_cast<uint32_t>(nowMs - lastInteractionMs);
      if (silence > longestSilenceMs) {
        longestSilenceMs = silence;
      }
    }
    lastInteractionMs = static_cast<uint32_t>(nowMs);
  }
};
