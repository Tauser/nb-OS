#pragma once

#include "../core/event_types.h"
#include "expression_types.h"

#include <stdint.h>

enum class ActionTargetChannel : uint8_t {
  Face = 0,
  Motion,
  Led,
  Audio
};

enum class ActionMotionCommand : uint8_t {
  None = 0,
  Center,
  IdleSway,
  CuriousLeft,
  CuriousRight,
  SoftListen,
  YawLeft,
  YawRight,
  TiltLeft,
  TiltRight
};

struct ActionIntent {
  ActionTargetChannel target = ActionTargetChannel::Face;
  uint8_t priority = 0;
  unsigned long holdMs = 0;
  EventSource source = EventSource::Unknown;
  const char* reason = "unspecified";

  struct FacePayload {
    ExpressionType expression = ExpressionType::Neutral;
    EyeAnimPriority animPriority = EyeAnimPriority::Idle;
  } face;

  struct MotionPayload {
    ActionMotionCommand command = ActionMotionCommand::None;
  } motion;
};
