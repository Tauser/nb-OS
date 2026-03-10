#include "power_service.h"

#include "../../config/hardware_config.h"
#include "../../models/event.h"
#include "../../utils/math_utils.h"
#include <Arduino.h>

PowerService::PowerService(PowerHAL& powerHal, EventBus& eventBus)
    : powerHal_(powerHal), eventBus_(eventBus) {}

void PowerService::init() {
  powerHal_.init();

  const unsigned long nowMs = millis();
  lastSampleMs_ = nowMs;
  lastStatusPublishMs_ = 0;

  state_.batteryPercent = HardwareConfig::Power::SIM_BATTERY_PERCENT;
  state_.charging = HardwareConfig::Power::SIM_CHARGING;
  state_.mode = decideMode(state_.batteryPercent, state_.charging);

  publishModeEvent(state_.mode, nowMs);
  publishChargingEvent(state_.charging, nowMs);
  publishStatusEvent(nowMs);
}

void PowerService::update(unsigned long nowMs) {
  if (nowMs - lastSampleMs_ < HardwareConfig::Power::SAMPLE_INTERVAL_MS) {
    return;
  }
  lastSampleMs_ = nowMs;

  PowerRawReading raw;
  if (!powerHal_.sample(raw) || !raw.valid) {
    return;
  }

  const bool prevCharging = state_.charging;
  const PowerMode prevMode = state_.mode;

  state_.batteryMillivolts = raw.batteryMillivolts;
  state_.batteryPercent = batteryPercentFromMillivolts(raw.batteryMillivolts);
  state_.charging = raw.charging;
  state_.mode = decideMode(state_.batteryPercent, state_.charging);

  if (state_.charging != prevCharging) {
    publishChargingEvent(state_.charging, nowMs);
  }

  if (state_.mode != prevMode) {
    publishModeEvent(state_.mode, nowMs);
  }

  if (nowMs - lastStatusPublishMs_ >= HardwareConfig::Power::PUBLISH_STATUS_INTERVAL_MS) {
    publishStatusEvent(nowMs);
  }
}

int PowerService::getBatteryPercent() const {
  return state_.batteryPercent;
}

bool PowerService::isCharging() const {
  return state_.charging;
}

PowerMode PowerService::getPowerMode() const {
  return state_.mode;
}

const PowerState& PowerService::getPowerState() const {
  return state_;
}

int PowerService::batteryPercentFromMillivolts(int mv) const {
  const int clampedMv = MathUtils::clamp(mv,
                                         HardwareConfig::Power::BATTERY_EMPTY_MV,
                                         HardwareConfig::Power::BATTERY_FULL_MV);
  const int span = HardwareConfig::Power::BATTERY_FULL_MV - HardwareConfig::Power::BATTERY_EMPTY_MV;
  if (span <= 0) {
    return 100;
  }

  return ((clampedMv - HardwareConfig::Power::BATTERY_EMPTY_MV) * 100) / span;
}

PowerMode PowerService::decideMode(int batteryPercent, bool charging) const {
  if (charging) {
    return PowerMode::Charging;
  }

  if (state_.mode == PowerMode::Sleep && batteryPercent >= HardwareConfig::Power::SLEEP_EXIT_PERCENT) {
    return (batteryPercent <= HardwareConfig::Power::LOW_POWER_ENTER_PERCENT)
               ? PowerMode::LowPower
               : PowerMode::Normal;
  }

  if (batteryPercent <= HardwareConfig::Power::SLEEP_ENTER_PERCENT) {
    return PowerMode::Sleep;
  }

  if (state_.mode == PowerMode::LowPower) {
    if (batteryPercent >= HardwareConfig::Power::LOW_POWER_EXIT_PERCENT) {
      return PowerMode::Normal;
    }
    return PowerMode::LowPower;
  }

  if (batteryPercent <= HardwareConfig::Power::LOW_POWER_ENTER_PERCENT) {
    return PowerMode::LowPower;
  }

  return PowerMode::Normal;
}

void PowerService::publishStatusEvent(unsigned long nowMs) {
  Event event;
  event.type = EventType::EVT_POWER_STATUS;
  event.source = EventSource::PowerService;
  event.timestamp = nowMs;

  PowerStatusPayload payload;
  payload.batteryPercent = state_.batteryPercent;
  payload.charging = state_.charging;
  payload.mode = static_cast<int>(state_.mode);
  payload.batteryMillivolts = state_.batteryMillivolts;
  event.setPowerStatus(payload);

  eventBus_.publish(event);

  lastStatusPublishMs_ = nowMs;
}

void PowerService::publishChargingEvent(bool charging, unsigned long nowMs) {
  Event event;
  event.type = EventType::EVT_CHARGING_STATE_CHANGED;
  event.source = EventSource::PowerService;
  event.value = charging ? 1 : 0;
  event.timestamp = nowMs;
  eventBus_.publish(event);
}

void PowerService::publishModeEvent(PowerMode mode, unsigned long nowMs) {
  Event event;
  event.type = EventType::EVT_POWER_MODE_CHANGED;
  event.source = EventSource::PowerService;
  event.value = static_cast<int>(mode);
  event.timestamp = nowMs;
  eventBus_.publish(event);
}

