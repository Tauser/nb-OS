#pragma once

#include "../models/expression_types.h"

class IFaceController {
public:
  virtual ~IFaceController() = default;
  virtual void requestExpression(ExpressionType expression,
                                 EyeAnimPriority priority,
                                 unsigned long holdMs = 0) = 0;
};
