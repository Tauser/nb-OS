#include "sensor_hal.h"

#include <Arduino.h>

SensorHAL::SensorHAL(Mpu6050Driver& imuDriver, TouchDriver& touchDriver)
    : imuDriver_(imuDriver), touchDriver_(touchDriver) {}

void SensorHAL::init() {
  imuDriver_.init();
  touchDriver_.init();
}

bool SensorHAL::sample(SensorSnapshot& outSnapshot) {
  outSnapshot = SensorSnapshot{};
  outSnapshot.timestampMs = millis();

  bool hasAnyData = false;

  ImuReading imu;
  if (imuDriver_.readAccel(imu)) {
    outSnapshot.imu = imu;
    hasAnyData = true;
  }

  TouchReading touch;
  if (touchDriver_.read(touch)) {
    outSnapshot.touch = touch;
    hasAnyData = true;
  }

  return hasAnyData;
}
