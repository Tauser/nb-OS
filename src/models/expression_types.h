#pragma once

#include <stdint.h>

enum class ExpressionType : uint8_t {
  Neutral = 0,
  Curiosity,
  FaceRecognized,
  BatteryAlert,
  Angry,
  Sad,
  Blink
};

enum class EyeAnimPriority : uint8_t {
  Idle = 0,
  Emotion,
  Social,
  Alert,
  Critical
};

struct EmotionPreset {
  float tiltDeg = 0.0f;
  float squashY = 1.0f;
  float stretchX = 1.0f;
  float upperLid = 0.18f;
  float lowerLid = 0.10f;
};
