#pragma once

#include "../drivers/imu/mpu6050_driver.h"
#include "../drivers/touch/touch_driver.h"
#include "../models/sensor_snapshot.h"

class SensorHAL {
public:
  SensorHAL(Mpu6050Driver& imuDriver, TouchDriver& touchDriver);

  void init();
  bool sample(SensorSnapshot& outSnapshot);

private:
  Mpu6050Driver& imuDriver_;
  TouchDriver& touchDriver_;
};
