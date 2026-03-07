#pragma once

#include <Arduino.h>

namespace Logger {
  inline void info(const char* tag, const char* msg) {
    Serial.print("[INFO][");
    Serial.print(tag);
    Serial.print("] ");
    Serial.println(msg);
  }

  inline void warn(const char* tag, const char* msg) {
    Serial.print("[WARN][");
    Serial.print(tag);
    Serial.print("] ");
    Serial.println(msg);
  }

  inline void error(const char* tag, const char* msg) {
    Serial.print("[ERROR][");
    Serial.print(tag);
    Serial.print("] ");
    Serial.println(msg);
  }
}
