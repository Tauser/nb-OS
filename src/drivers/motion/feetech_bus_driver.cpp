#include "feetech_bus_driver.h"

#include "../../config/hardware_config.h"
#include "../../config/hardware_pins.h"
#include <Arduino.h>

namespace {
constexpr uint8_t kHeader = 0xFF;
constexpr uint8_t kInstructionWrite = 0x03;
constexpr uint8_t kRegGoalPositionL = 0x2A;
HardwareSerial gServoSerial(2);
}

bool FeetechBusDriver::init() {
  gServoSerial.begin(HardwareConfig::Motion::BUS_BAUDRATE,
                     SERIAL_8N1,
                     -1,
                     HardwarePins::ServoBus::DATA);
  ready_ = true;
  return true;
}

bool FeetechBusDriver::writePosition(uint8_t servoId, uint16_t position, uint16_t moveTimeMs) {
  if (!ready_) {
    return false;
  }

  const uint16_t clampedPos = static_cast<uint16_t>(
      constrain(position, HardwareConfig::Motion::SERVO_POS_MIN, HardwareConfig::Motion::SERVO_POS_MAX));

  const uint8_t params[5] = {
      kRegGoalPositionL,
      static_cast<uint8_t>(clampedPos & 0xFF),
      static_cast<uint8_t>((clampedPos >> 8) & 0xFF),
      static_cast<uint8_t>(moveTimeMs & 0xFF),
      static_cast<uint8_t>((moveTimeMs >> 8) & 0xFF)};

  return writePacket(servoId, kInstructionWrite, params, sizeof(params));
}

bool FeetechBusDriver::isReady() const {
  return ready_;
}

bool FeetechBusDriver::writePacket(uint8_t servoId,
                                   uint8_t instruction,
                                   const uint8_t* params,
                                   uint8_t paramLength) {
  if (!ready_ || params == nullptr) {
    return false;
  }

  const uint8_t length = static_cast<uint8_t>(paramLength + 2);
  uint8_t checksumBase = static_cast<uint8_t>(servoId + length + instruction);

  gServoSerial.write(kHeader);
  gServoSerial.write(kHeader);
  gServoSerial.write(servoId);
  gServoSerial.write(length);
  gServoSerial.write(instruction);

  for (uint8_t i = 0; i < paramLength; ++i) {
    gServoSerial.write(params[i]);
    checksumBase = static_cast<uint8_t>(checksumBase + params[i]);
  }

  const uint8_t checksum = static_cast<uint8_t>(~checksumBase);
  gServoSerial.write(checksum);
  gServoSerial.flush();
  return true;
}
