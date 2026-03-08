#pragma once

#include <stdint.h>

enum class LocalIntent : uint8_t {
  None = 0,
  Hello,
  Status,
  Sleep,
  Wake,
  Photo
};

struct VoiceCommandFrame {
  uint16_t durationMs = 0;
  uint16_t avgLevel = 0;  // 0..1000
  uint16_t peakLevel = 0; // 0..1000
  uint8_t burstCount = 1;
};

struct DialogueReply {
  const char* text = "";
  uint16_t toneHz = 0;
  uint16_t toneMs = 0;
};
