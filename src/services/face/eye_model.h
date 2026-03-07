#pragma once

struct EyeModel {
  int centerX = 0;
  int centerY = 0;
  int eyeRadius = 30;

  int pupilOffsetX = 0;
  int pupilOffsetY = 0;
  int pupilRadius = 8;

  float openness = 1.0f; // 1.0 = aberto, 0.0 = fechado
};