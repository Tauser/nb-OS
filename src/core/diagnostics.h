#pragma once
#include <Arduino.h>

class Diagnostics {
public:
  void begin();
  void printBanner() const;
  void logInfo(const char* message) const;
  void logError(const char* message) const;
  void printHeartbeat(unsigned long uptimeMs, const char* stateName) const;
};
