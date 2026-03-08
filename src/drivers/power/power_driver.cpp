#include "power_driver.h"

#include "../../config/hardware_config.h"
#include "../../config/hardware_pins.h"
#include <Arduino.h>

bool PowerDriver::init() {
  if (HardwarePins::Power::BATTERY_ADC >= 0) {
    pinMode(HardwarePins::Power::BATTERY_ADC, INPUT);
  }

  if (HardwarePins::Power::CHARGE_DETECT >= 0) {
    pinMode(HardwarePins::Power::CHARGE_DETECT, INPUT);
  }

  ready_ = true;
  return true;
}

bool PowerDriver::sample(PowerRawReading& outReading) const {
  outReading = PowerRawReading{};

  if (!ready_) {
    return false;
  }

  if (HardwarePins::Power::BATTERY_ADC < 0) {
    outReading.batteryMillivolts = static_cast<int>(
        static_cast<float>(HardwareConfig::Power::BATTERY_EMPTY_MV) +
        (static_cast<float>(HardwareConfig::Power::BATTERY_FULL_MV - HardwareConfig::Power::BATTERY_EMPTY_MV) *
         (static_cast<float>(HardwareConfig::Power::SIM_BATTERY_PERCENT) / 100.0f)));
    outReading.charging = HardwareConfig::Power::SIM_CHARGING;
    outReading.valid = true;
    return true;
  }

  const int raw = analogRead(HardwarePins::Power::BATTERY_ADC);
  int sensedMv = analogReadMilliVolts(HardwarePins::Power::BATTERY_ADC);
  if (sensedMv <= 0) {
    sensedMv = (raw * HardwareConfig::Power::ADC_REF_MV) / HardwareConfig::Power::ADC_MAX;
  }

  const float packMv = static_cast<float>(sensedMv) * HardwareConfig::Power::BATTERY_DIVIDER_RATIO;
  outReading.batteryMillivolts = static_cast<int>(packMv);

  if (HardwarePins::Power::CHARGE_DETECT >= 0) {
    const int level = digitalRead(HardwarePins::Power::CHARGE_DETECT);
    outReading.charging = (level == HardwareConfig::Power::CHARGE_DETECT_ACTIVE_LEVEL);
  } else {
    outReading.charging = HardwareConfig::Power::SIM_CHARGING;
  }

  outReading.valid = true;
  return true;
}

bool PowerDriver::isReady() const {
  return ready_;
}
