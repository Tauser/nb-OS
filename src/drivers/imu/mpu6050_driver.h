#pragma once

#include "../../models/imu_reading.h"
#include <stdint.h>

class Mpu6050Driver {
public:
  bool init();
  bool readAccel(ImuReading& outReading);
  bool isReady() const;

private:
  bool readRegister(uint8_t reg, uint8_t& outValue) const;
  bool readRegisters(uint8_t startReg, uint8_t* buffer, int length) const;

  bool ready_ = false;
};
