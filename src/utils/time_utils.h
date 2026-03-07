#pragma once

#include <Arduino.h>

namespace TimeUtils {
  inline bool elapsed(unsigned long now, unsigned long last, unsigned long interval) {
    return (now - last) >= interval;
  }
}
