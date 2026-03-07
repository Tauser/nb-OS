#include "diagnostics.h"

#ifndef NCOS_SIM_MODE
#define NCOS_SIM_MODE 0
#endif

void Diagnostics::begin() {
  Serial.begin(115200);
#if NCOS_SIM_MODE
  delay(100);
#else
  delay(500);
#endif
}

void Diagnostics::printBanner() const {
  Serial.println();
  Serial.println("==================================================");
#if NCOS_SIM_MODE
  Serial.println(" ROBOT DESKTOP - SIM MODE (WOKWI)");
#else
  Serial.println(" ROBOT DESKTOP - ETAPA 2A");
#endif
  Serial.println(" ST7789 REAL + LOVYANGFX");
  Serial.println("==================================================");
}

void Diagnostics::logInfo(const char* message) const {
  Serial.print("[INFO] ");
  Serial.println(message);
}

void Diagnostics::logError(const char* message) const {
  Serial.print("[ERROR] ");
  Serial.println(message);
}

void Diagnostics::printHeartbeat(unsigned long uptimeMs, const char* stateName) const {
  Serial.print("[HB] uptime_ms=");
  Serial.print(uptimeMs);
  Serial.print(" state=");
  Serial.println(stateName);
}