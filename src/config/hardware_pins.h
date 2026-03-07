#pragma once

// Set to 1 when running Wokwi/simulation wiring.
#ifndef NCOS_PIN_PROFILE_SIM
#define NCOS_PIN_PROFILE_SIM 0
#endif

namespace HardwarePins {

namespace Display {
#if NCOS_PIN_PROFILE_SIM
  // Wokwi profile (diagram.json)
  constexpr int SCLK = 21;
  constexpr int MOSI = 47;
  constexpr int MISO = -1;
  constexpr int DC = 48;
  constexpr int CS = 14;
  constexpr int RST = -1;
  constexpr int BL = 16;
#else
  // Real hardware profile (Freenove ESP32-S3-WROOM CAM + ST7789 2")
  constexpr int SCLK = 47;
  constexpr int MOSI = 21;
  constexpr int MISO = -1;
  constexpr int DC = 48;
  constexpr int CS = 14;
  constexpr int RST = 2;
  constexpr int BL = -1; // Backlight tied to 3V3.
#endif
}

namespace Camera {
  // OV2640 is connected through board camera connector.
  constexpr int PWDN = -1;
  constexpr int RESET = -1;
}

namespace AudioIn {
  // I2S shared clock lines with AudioOut.
  constexpr int BCLK = 43;
  constexpr int WS = 44;
  constexpr int DATA = 42; // INMP441 SD
}

namespace AudioOut {
  constexpr int BCLK = 43;
  constexpr int WS = 44;
  constexpr int DATA = 1; // MAX98357A DIN
}

namespace I2C {
  constexpr int SDA = 41; // MPU6050 SDA
  constexpr int SCL = 40; // MPU6050 SCL
}

namespace ServoBus {
  // Dedicated half-duplex bus line for FE-TTLinker-Mini.
  constexpr int DATA = 39;
}

namespace Touch {
  // Capacitive copper tape input.
  constexpr int DATA = 38;
}

} // namespace HardwarePins