#pragma once

#include "../../interfaces/i_display_port.h"
#include "eye_model.h"

class FaceRenderer {
public:
  explicit FaceRenderer(IDisplayPort& displayPort);

  bool render(const EyeModel& leftEye, const EyeModel& rightEye);

private:
  struct Rect {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    bool valid = false;
  };

  Rect getEyeDirtyRect(const EyeModel& eye) const;
  Rect mergeRect(const Rect& a, const Rect& b) const;
  bool hasMeaningfulChange(const EyeModel& leftEye, const EyeModel& rightEye) const;

  IDisplayPort& displayPort_;
  EyeModel lastLeft_{};
  EyeModel lastRight_{};
  bool hasLastFrame_ = false;
};
