#pragma once

#include "../../hal/display_hal.h"
#include "face_renderer.h"
#include "eye_model.h"

class FaceService {
public:
  explicit FaceService(DisplayHAL& displayHal);

  void init();
  void update(unsigned long nowMs);

private:
  void updateBlink(unsigned long nowMs);
  void updateIdleLook(unsigned long nowMs);
  void scheduleNextBlink(unsigned long nowMs);

  DisplayHAL& displayHal_;
  FaceRenderer renderer_;

  EyeModel leftEye_;
  EyeModel rightEye_;

  unsigned long nextBlinkAtMs_ = 0;
  unsigned long blinkStartMs_ = 0;
  bool blinking_ = false;

  unsigned long lastLookChangeMs_ = 0;
};