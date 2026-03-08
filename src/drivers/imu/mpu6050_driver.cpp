#include "mpu6050_driver.h"

#include "../../config/hardware_config.h"
#include "../../config/hardware_pins.h"
#include <Arduino.h>
#include <Wire.h>

namespace {
constexpr uint8_t kRegWhoAmI = 0x75;
constexpr uint8_t kRegPowerMgmt1 = 0x6B;
constexpr uint8_t kRegAccelStart = 0x3B;
}

bool Mpu6050Driver::init() {
  Wire.begin(HardwarePins::I2C::SDA, HardwarePins::I2C::SCL);

  const uint8_t addr = static_cast<uint8_t>(HardwareConfig::IMU::I2C_ADDRESS);

  Wire.beginTransmission(addr);
  Wire.write(kRegPowerMgmt1);
  Wire.write(0x00);
  if (Wire.endTransmission() != 0) {
    ready_ = false;
    return false;
  }

  uint8_t whoAmI = 0;
  if (!readRegister(kRegWhoAmI, whoAmI)) {
    ready_ = false;
    return false;
  }

  // Common MPU6050 identities observed in modules/clones.
  ready_ = (whoAmI == 0x68 || whoAmI == 0x69);
  return ready_;
}

bool Mpu6050Driver::readAccel(ImuReading& outReading) {
  outReading = ImuReading{};

  if (!ready_) {
    return false;
  }

  uint8_t raw[6] = {0};
  if (!readRegisters(kRegAccelStart, raw, 6)) {
    return false;
  }

  const int16_t axRaw = static_cast<int16_t>((raw[0] << 8) | raw[1]);
  const int16_t ayRaw = static_cast<int16_t>((raw[2] << 8) | raw[3]);
  const int16_t azRaw = static_cast<int16_t>((raw[4] << 8) | raw[5]);

  outReading.accelXg = static_cast<float>(axRaw) * HardwareConfig::IMU::ACCEL_G_PER_LSB;
  outReading.accelYg = static_cast<float>(ayRaw) * HardwareConfig::IMU::ACCEL_G_PER_LSB;
  outReading.accelZg = static_cast<float>(azRaw) * HardwareConfig::IMU::ACCEL_G_PER_LSB;
  outReading.valid = true;
  return true;
}

bool Mpu6050Driver::isReady() const {
  return ready_;
}

bool Mpu6050Driver::readRegister(uint8_t reg, uint8_t& outValue) const {
  return readRegisters(reg, &outValue, 1);
}

bool Mpu6050Driver::readRegisters(uint8_t startReg, uint8_t* buffer, int length) const {
  if (!ready_ && startReg != kRegWhoAmI) {
    return false;
  }

  const uint8_t addr = static_cast<uint8_t>(HardwareConfig::IMU::I2C_ADDRESS);
  Wire.beginTransmission(addr);
  Wire.write(startReg);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  const size_t requested = Wire.requestFrom(static_cast<uint8_t>(addr), static_cast<size_t>(length), true);
  if (requested != length) {
    return false;
  }

  for (int i = 0; i < length; ++i) {
    if (!Wire.available()) {
      return false;
    }
    buffer[i] = static_cast<uint8_t>(Wire.read());
  }

  return true;
}
