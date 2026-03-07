#pragma once

#include "../../hal/display_hal.h"
#include "eye_model.h"

class FaceRenderer {
public:
  explicit FaceRenderer(DisplayHAL& displayHal);

  void render(const EyeModel& leftEye, const EyeModel& rightEye);

private:
  DisplayHAL& displayHal_;
};