#pragma once

namespace HardwareConfig {

namespace Display {
  constexpr int WIDTH  = 240;
  constexpr int HEIGHT = 240;

  constexpr int OFFSET_X = 0;
  constexpr int OFFSET_Y = 0;
  constexpr int ROTATION = 1;

  constexpr int SPI_WRITE_FREQ = 40000000;
  constexpr int SPI_READ_FREQ  = 16000000;

  constexpr bool INVERT = false;
  constexpr bool RGB_ORDER = false;
}

namespace Face {
  constexpr int LEFT_EYE_X   = 80;
  constexpr int RIGHT_EYE_X  = 160;
  constexpr int EYE_Y        = 120;
  constexpr int EYE_RADIUS   = 30;

  constexpr int PUPIL_RADIUS = 8;
  constexpr int MAX_PUPIL_OFFSET_X = 6;
  constexpr int MAX_PUPIL_OFFSET_Y = 4;

  constexpr unsigned long BLINK_MIN_INTERVAL_MS = 3500;
  constexpr unsigned long BLINK_MAX_INTERVAL_MS = 6500;
  constexpr unsigned long LOOK_INTERVAL_MS = 1200;
}

namespace System {
  constexpr unsigned long HEARTBEAT_INTERVAL_MS = 1000;
  constexpr unsigned long FACE_FRAME_INTERVAL_MS = 33;
}

}