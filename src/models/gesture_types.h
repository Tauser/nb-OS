#pragma once

#include <stdint.h>

enum class GestureType : uint8_t {
  None = 0,
  CuriousLeft,
  HappyTilt,
  ShyTurn,
  SoftListen,
  SurprisedJerk
};
