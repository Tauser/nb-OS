#include "diagnostics.h"

void Diagnostics::begin() {
  Serial.begin(115200);
  delay(500);
}

void Diagnostics::printBanner() const {
  Serial.println();
  Serial.println("==================================================");
  Serial.println(" ROBOT DESKTOP - ETAPA 2A");
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
