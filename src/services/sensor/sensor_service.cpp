#include "sensor_service.h"

#include "../../config/hardware_config.h"
#include "../../models/event.h"
#include <math.h>

namespace {
float accelMagnitude(const ImuReading& imu) {
  return sqrtf((imu.accelXg * imu.accelXg) +
               (imu.accelYg * imu.accelYg) +
               (imu.accelZg * imu.accelZg));
}

int toMilliG(float gValue) {
  return static_cast<int>(gValue * 1000.0f);
}
}

SensorService::SensorService(SensorHAL& sensorHal, EventBus& eventBus)
    : sensorHal_(sensorHal), eventBus_(eventBus) {}

void SensorService::init() {
  sensorHal_.init();
}

void SensorService::update(unsigned long nowMs) {
  SensorSnapshot snapshot;
  if (!sensorHal_.sample(snapshot)) {
    return;
  }

  const unsigned long debounceMs = HardwareConfig::SensorEvents::EVENT_DEBOUNCE_MS;

  if (snapshot.touch.valid) {
    const bool risingEdge = snapshot.touch.touched && !lastTouchState_;
    if (risingEdge && (nowMs - lastTouchEventMs_ >= debounceMs)) {
      lastTouchEventMs_ = nowMs;
      publishEvent(EventType::EVT_TOUCH, snapshot.touch.rawValue, nowMs);
    }
    lastTouchState_ = snapshot.touch.touched;
  }

  if (snapshot.imu.valid) {
    if (hasLastSnapshot_ && lastSnapshot_.imu.valid) {
      const float delta = fabsf(accelMagnitude(snapshot.imu) - accelMagnitude(lastSnapshot_.imu));
      if (delta >= HardwareConfig::SensorEvents::SHAKE_DELTA_G &&
          (nowMs - lastShakeEventMs_ >= debounceMs)) {
        lastShakeEventMs_ = nowMs;
        publishEvent(EventType::EVT_SHAKE, toMilliG(delta), nowMs);
      }
    }

    const float tilt = fmaxf(fabsf(snapshot.imu.accelXg), fabsf(snapshot.imu.accelYg));
    if (tilt >= HardwareConfig::SensorEvents::TILT_G_THRESHOLD &&
        (nowMs - lastTiltEventMs_ >= debounceMs)) {
      lastTiltEventMs_ = nowMs;
      publishEvent(EventType::EVT_TILT, toMilliG(tilt), nowMs);
    }

    const float absZ = fabsf(snapshot.imu.accelZg);
    if (absZ <= HardwareConfig::SensorEvents::FALL_Z_G_THRESHOLD &&
        (nowMs - lastFallEventMs_ >= debounceMs)) {
      lastFallEventMs_ = nowMs;
      publishEvent(EventType::EVT_FALL, toMilliG(absZ), nowMs);
    }
  }

  lastSnapshot_ = snapshot;
  hasLastSnapshot_ = true;
}

void SensorService::publishEvent(EventType type, int value, unsigned long nowMs) {
  Event event;
  event.type = type;
  event.source = EventSource::SensorService;
  event.value = value;
  event.timestamp = nowMs;
  eventBus_.publish(event);
}
