#pragma once

#include <stddef.h>

#include "../models/expression_types.h"
#include "../models/face_gaze_target.h"
#include "../models/face_render_state.h"

class IFaceController {
public:
  virtual ~IFaceController() = default;

  virtual void requestExpression(ExpressionType expression,
                                 EyeAnimPriority priority,
                                 unsigned long holdMs = 0) = 0;

  virtual void requestGazeTarget(const FaceGazeTarget& target) {
    (void)target;
  }

  virtual void clearGazeTarget() {}

  virtual bool requestClip(FaceClipKind kind, bool allowPreempt = true) {
    (void)kind;
    (void)allowPreempt;
    return false;
  }

  virtual bool cancelClip() {
    return false;
  }

  // Optional runtime tuning API used by simulation/editor tooling.
  virtual bool tunerSetEnabled(bool enabled) {
    (void)enabled;
    return false;
  }

  virtual bool tunerSetPreset(const char* presetName) {
    (void)presetName;
    return false;
  }

  virtual bool tunerSetParam(const char* key, float value) {
    (void)key;
    (void)value;
    return false;
  }

  virtual bool tunerReset() {
    return false;
  }

  virtual bool tunerGetStatus(char* out, size_t outSize) const {
    (void)out;
    (void)outSize;
    return false;
  }
};
