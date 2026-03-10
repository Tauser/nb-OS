#pragma once

#include <stddef.h>

#include "../models/expression_types.h"

class IFaceController {
public:
  virtual ~IFaceController() = default;
  virtual void requestExpression(ExpressionType expression,
                                 EyeAnimPriority priority,
                                 unsigned long holdMs = 0) = 0;

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


