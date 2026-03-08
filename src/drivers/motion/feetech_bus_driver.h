#pragma once

#include <stdint.h>

class FeetechBusDriver {
public:
  bool init();
  bool writePosition(uint8_t servoId, uint16_t position, uint16_t moveTimeMs);
  bool isReady() const;

private:
  bool writePacket(uint8_t servoId,
                   uint8_t instruction,
                   const uint8_t* params,
                   uint8_t paramLength);

  bool ready_ = false;
};
