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
  constexpr int BL = -1;
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
#if NCOS_PIN_PROFILE_SIM
  constexpr int PWDN = -1;
  constexpr int RESET = -1;
  constexpr int XCLK = -1;
  constexpr int SIOD = -1;
  constexpr int SIOC = -1;
  constexpr int Y9 = -1;
  constexpr int Y8 = -1;
  constexpr int Y7 = -1;
  constexpr int Y6 = -1;
  constexpr int Y5 = -1;
  constexpr int Y4 = -1;
  constexpr int Y3 = -1;
  constexpr int Y2 = -1;
  constexpr int VSYNC = -1;
  constexpr int HREF = -1;
  constexpr int PCLK = -1;
#else
  // Freenove ESP32-S3-WROOM CAM N16R8 default OV2640 bus mapping.
  constexpr int PWDN = -1;
  constexpr int RESET = -1;
  constexpr int XCLK = 15;
  constexpr int SIOD = 4;
  constexpr int SIOC = 5;
  constexpr int Y9 = 16;
  constexpr int Y8 = 17;
  constexpr int Y7 = 18;
  constexpr int Y6 = 12;
  constexpr int Y5 = 10;
  constexpr int Y4 = 8;
  constexpr int Y3 = 9;
  constexpr int Y2 = 11;
  constexpr int VSYNC = 6;
  constexpr int HREF = 7;
  constexpr int PCLK = 13;
#endif
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


namespace Led {
#if NCOS_PIN_PROFILE_SIM
  // Two addressable RGB LEDs in chain on a single DATA line.
  constexpr int DATA = 6;
#else
  // Real hardware: two addressable RGB LEDs in chain on a single DATA line.
  constexpr int DATA = 6;
#endif
}
namespace Power {
#if NCOS_PIN_PROFILE_SIM
  constexpr int BATTERY_ADC = -1;
  constexpr int CHARGE_DETECT = -1;
#else
  // Battery sense via ADC pin with resistor divider.
  constexpr int BATTERY_ADC = 4;
  // Optional charging detect pin from charger IC status.
  constexpr int CHARGE_DETECT = 5;
#endif
}
} // namespace HardwarePins









