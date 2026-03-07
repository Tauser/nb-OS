#pragma once

#ifndef NCOS_SIM_MODE
#define NCOS_SIM_MODE 0
#endif

#ifndef NCOS_SIM_ULTRA
#define NCOS_SIM_ULTRA 0
#endif

namespace HardwareConfig {

namespace Display {
  constexpr int WIDTH = 320;
  constexpr int HEIGHT = 240;

  constexpr int OFFSET_X = 0;
  constexpr int OFFSET_Y = 0;
  constexpr int ROTATION = 1;

#if NCOS_SIM_MODE
  // Turbo sim profile for Wokwi stability.
  constexpr int SPI_WRITE_FREQ = 20000000;
  constexpr int SPI_READ_FREQ = 10000000;
  constexpr bool INVERT = false;
  constexpr bool RGB_ORDER = false;
#else
  constexpr int SPI_WRITE_FREQ = 40000000;
  constexpr int SPI_READ_FREQ = 16000000;
  constexpr bool INVERT = false;
  constexpr bool RGB_ORDER = true;
#endif
}

namespace Face {
  constexpr int LEFT_EYE_X = 120;
  constexpr int RIGHT_EYE_X = 200;
  constexpr int EYE_Y = 120;
#if NCOS_SIM_MODE
  constexpr int EYE_RADIUS = 32;
#else
  constexpr int EYE_RADIUS = 34;
#endif

  constexpr int PUPIL_RADIUS = 8;
  constexpr int MAX_PUPIL_OFFSET_X = 6;
  constexpr int MAX_PUPIL_OFFSET_Y = 4;

#if NCOS_SIM_MODE && NCOS_SIM_ULTRA
  constexpr unsigned long BLINK_MIN_INTERVAL_MS = 5000;
  constexpr unsigned long BLINK_MAX_INTERVAL_MS = 9000;
  constexpr unsigned long LOOK_INTERVAL_MS = 2400;
#else
  constexpr unsigned long BLINK_MIN_INTERVAL_MS = 3500;
  constexpr unsigned long BLINK_MAX_INTERVAL_MS = 6500;
  constexpr unsigned long LOOK_INTERVAL_MS = 1200;
#endif
}

namespace IMU {
  constexpr int I2C_ADDRESS = 0x68;
  constexpr float ACCEL_G_PER_LSB = 1.0f / 16384.0f;
}

namespace Touch {
#if NCOS_SIM_MODE
  constexpr int ANALOG_TOUCH_THRESHOLD = 1500;
#else
  constexpr int ANALOG_TOUCH_THRESHOLD = 1900;
#endif
}

namespace SensorEvents {
  constexpr float SHAKE_DELTA_G = 0.85f;
  constexpr float TILT_G_THRESHOLD = 0.45f;
  constexpr float FALL_Z_G_THRESHOLD = 0.30f;
  constexpr unsigned long EVENT_DEBOUNCE_MS = 350;
}

namespace System {
#if NCOS_SIM_MODE && NCOS_SIM_ULTRA
  constexpr unsigned long HEARTBEAT_INTERVAL_MS = 8000;
  constexpr unsigned long FACE_FRAME_INTERVAL_MS = 40; // 25 FPS
  constexpr unsigned long SENSOR_POLL_INTERVAL_MS = 120;
#elif NCOS_SIM_MODE
  constexpr unsigned long HEARTBEAT_INTERVAL_MS = 5000;
  constexpr unsigned long FACE_FRAME_INTERVAL_MS = 66;  // 15 FPS
  constexpr unsigned long SENSOR_POLL_INTERVAL_MS = 100;
#else
  constexpr unsigned long HEARTBEAT_INTERVAL_MS = 1000;
  constexpr unsigned long FACE_FRAME_INTERVAL_MS = 33;
  constexpr unsigned long SENSOR_POLL_INTERVAL_MS = 50;
#endif
}

} // namespace HardwareConfig
