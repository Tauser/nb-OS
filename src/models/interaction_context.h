#pragma once

struct InteractionContext {
  bool touchActive = false;
  bool voiceActive = false;
  bool motionDetected = false;
  unsigned long lastInteractionMs = 0;
};
