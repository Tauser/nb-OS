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

namespace Vision {
  constexpr unsigned int DARK_LUMA_THRESHOLD = 240;
  constexpr unsigned int MOTION_SCORE_THRESHOLD = 90;
  constexpr unsigned long EVENT_COOLDOWN_MS = 300;
}
namespace Cloud {
  constexpr unsigned long REQUEST_TIMEOUT_MS = 1600;
  constexpr unsigned long REMOTE_LATENCY_MS = 280;
  constexpr unsigned long REQUEST_COOLDOWN_MS = 500;
  constexpr unsigned int MAX_RETRIES = 1;
}
namespace AudioIn {
  constexpr unsigned long SAMPLE_RATE_HZ = 16000;
  constexpr unsigned int DMA_BUFFER_SAMPLES = 256;
  constexpr unsigned int FRAME_SAMPLES = 160; // 10 ms @ 16 kHz
  constexpr unsigned long READ_TIMEOUT_MS = 8;
}
namespace AudioOut {
  constexpr unsigned long SAMPLE_RATE_HZ = 16000;
  constexpr unsigned int DMA_BUFFER_SAMPLES = 256;
  constexpr unsigned long WRITE_TIMEOUT_MS = 25;

  constexpr float DEFAULT_AMPLITUDE = 0.18f;
  constexpr unsigned long SFX_COOLDOWN_MS = 120;

  constexpr unsigned int BOOT_FREQ_HZ = 880;
  constexpr unsigned int BOOT_TONE_MS = 110;
  constexpr unsigned int TOUCH_FREQ_HZ = 1200;
  constexpr unsigned int TOUCH_TONE_MS = 60;
  constexpr unsigned int ALERT_FREQ_HZ = 340;
  constexpr unsigned int ALERT_TONE_MS = 180;

  constexpr unsigned int NOTIFY_FREQ_A_HZ = 990;
  constexpr unsigned int NOTIFY_FREQ_B_HZ = 1320;
  constexpr unsigned int NOTIFY_TONE_MS = 60;
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

namespace Motion {
  constexpr int BUS_BAUDRATE = 1000000;
  constexpr int YAW_SERVO_ID = 1;
  constexpr int TILT_SERVO_ID = 2;

  constexpr int SERVO_POS_MIN = 0;
  constexpr int SERVO_POS_MAX = 4095;
  constexpr int SERVO_POS_CENTER = 2048;
  constexpr float SERVO_POS_PER_DEG = 11.3778f;

  constexpr float YAW_MIN_DEG = -35.0f;
  constexpr float YAW_MAX_DEG = 35.0f;
  constexpr float TILT_MIN_DEG = -22.0f;
  constexpr float TILT_MAX_DEG = 22.0f;

  constexpr unsigned long DEFAULT_MOVE_MS = 320;
  constexpr unsigned long QUICK_MOVE_MS = 220;

  constexpr float YAW_LOOK_DEG = 20.0f;
  constexpr float TILT_LOOK_DEG = 12.0f;
  constexpr float CURIOUS_YAW_DEG = 18.0f;
  constexpr float CURIOUS_TILT_DEG = -10.0f;
  constexpr float SOFT_LISTEN_TILT_DEG = 14.0f;

  constexpr float IDLE_SWAY_YAW_DEG = 8.0f;
  constexpr unsigned long IDLE_SWAY_MOVE_MS = 420;
  constexpr unsigned long IDLE_SWAY_INTERVAL_MS = 1800;
}

namespace MotionReactions {
  constexpr unsigned long REACTION_COOLDOWN_MS = 800;

  constexpr float TOUCH_EASE_POWER = 0.70f;
  constexpr float SHAKE_EASE_POWER = 0.85f;
  constexpr float TILT_EASE_POWER = 0.75f;
  constexpr float FALL_EASE_POWER = 0.65f;

  constexpr float TOUCH_DEADZONE = 0.08f;
  constexpr float SHAKE_DEADZONE = 0.12f;
  constexpr float TILT_DEADZONE = 0.10f;
  constexpr float FALL_DEADZONE = 0.12f;

  constexpr int TOUCH_EXCESS_MAX = 1200;
  constexpr int SHAKE_MG_MAX = 2500;
  constexpr int TILT_MG_MAX = 1400;
  constexpr int FALL_MG_MAX = 800;

  constexpr float TOUCH_MIN_YAW_DEG = 8.0f;
  constexpr float TOUCH_MAX_YAW_DEG = 22.0f;
  constexpr float TOUCH_MIN_TILT_DEG = -4.0f;
  constexpr float TOUCH_MAX_TILT_DEG = -12.0f;

  constexpr float TILT_MIN_DEG = 6.0f;
  constexpr float TILT_MAX_DEG = 16.0f;

  constexpr unsigned long REACTION_MIN_MS = 180;
  constexpr unsigned long REACTION_MAX_MS = 420;
}

namespace VAD {
  constexpr float NOISE_ALPHA_IDLE = 0.08f;
  constexpr float NOISE_ALPHA_ACTIVE = 0.02f;
  constexpr float ENERGY_THRESHOLD_DELTA = 0.018f;
  constexpr unsigned int START_CONSECUTIVE_FRAMES = 3;
  constexpr unsigned int END_CONSECUTIVE_FRAMES = 8;
}

namespace Voice {
  constexpr unsigned long ACTIVITY_PUBLISH_INTERVAL_MS = 220;
  constexpr unsigned long BURST_GAP_MS = 320;
}
namespace Intent {
  constexpr unsigned int MIN_COMMAND_MS = 120;
  constexpr unsigned int HELLO_MAX_MS = 420;
  constexpr unsigned int HELLO_MAX_LEVEL = 190;

  constexpr unsigned int WAKE_MAX_MS = 520;
  constexpr unsigned int WAKE_MIN_PEAK_LEVEL = 260;

  constexpr unsigned int STATUS_MIN_MS = 420;
  constexpr unsigned int STATUS_MAX_MS = 1050;

  constexpr unsigned int SLEEP_MIN_MS = 950;
  constexpr unsigned int SLEEP_MAX_LEVEL = 180;

  constexpr unsigned int PHOTO_BURST_MIN = 2;
}

namespace Dialogue {
  constexpr unsigned int HELLO_TONE_HZ = 980;
  constexpr unsigned int HELLO_TONE_MS = 80;
  constexpr unsigned int STATUS_TONE_HZ = 760;
  constexpr unsigned int STATUS_TONE_MS = 90;
  constexpr unsigned int SLEEP_TONE_HZ = 420;
  constexpr unsigned int SLEEP_TONE_MS = 160;
  constexpr unsigned int WAKE_TONE_HZ = 1180;
  constexpr unsigned int WAKE_TONE_MS = 100;
  constexpr unsigned int PHOTO_TONE_HZ = 1500;
  constexpr unsigned int PHOTO_TONE_MS = 70;
}
namespace Emotion {
  constexpr float DECAY_VALENCE_PER_S = 0.12f;
  constexpr float DECAY_AROUSAL_PER_S = 0.28f;
  constexpr float DECAY_CURIOSITY_PER_S = 0.18f;
  constexpr float DECAY_ATTENTION_PER_S = 0.24f;
  constexpr float DECAY_BOND_PER_S = 0.06f;
  constexpr float DECAY_ENERGY_PER_S = 0.10f;

  constexpr float TOUCH_D_VALENCE = 0.10f;
  constexpr float TOUCH_D_AROUSAL = 0.05f;
  constexpr float TOUCH_D_CURIOSITY = 0.04f;
  constexpr float TOUCH_D_ATTENTION = 0.08f;
  constexpr float TOUCH_D_BOND = 0.04f;

  constexpr float SHAKE_D_VALENCE = -0.05f;
  constexpr float SHAKE_D_AROUSAL = 0.20f;
  constexpr float SHAKE_D_ATTENTION = 0.10f;

  constexpr float TILT_D_CURIOSITY = 0.06f;
  constexpr float TILT_D_ATTENTION = 0.06f;

  constexpr float FALL_D_VALENCE = -0.10f;
  constexpr float FALL_D_AROUSAL = 0.18f;
  constexpr float FALL_D_ENERGY = -0.06f;

  constexpr float IDLE_D_CURIOSITY = -0.03f;
  constexpr float IDLE_D_ATTENTION = -0.02f;
  constexpr float IDLE_D_AROUSAL = -0.01f;

  constexpr float VOICE_D_ATTENTION = 0.10f;
  constexpr float VOICE_D_AROUSAL = 0.07f;
  constexpr float VOICE_D_BOND = 0.04f;

  constexpr float CHANGE_PUBLISH_THRESHOLD = 0.03f;
  constexpr unsigned long CHANGE_PUBLISH_MIN_INTERVAL_MS = 250;
}

namespace EmotionOutput {
  constexpr unsigned long FACE_APPLY_INTERVAL_MS = 350;
  constexpr float FACE_ENERGY_LOW = 0.30f;
  constexpr float FACE_VALENCE_NEG = -0.35f;
  constexpr float FACE_CURIOSITY_HIGH = 0.65f;
  constexpr float FACE_VALENCE_POS = 0.35f;
  constexpr unsigned long FACE_EXPRESSION_HOLD_MS = 500;
  constexpr float FACE_ACTIVITY_AROUSAL_WEIGHT = 0.60f;
  constexpr float FACE_ACTIVITY_ENERGY_WEIGHT = 0.40f;
  constexpr float FACE_ACTIVITY_GAIN = 0.12f;
  constexpr float FACE_LOW_ENERGY_GAIN = 0.10f;
  constexpr float FACE_OPENNESS_MIN = 0.78f;
  constexpr float FACE_OPENNESS_MAX = 1.15f;

  constexpr unsigned long MOTION_APPLY_INTERVAL_MS = 300;
  constexpr float MOTION_ACTIVE_YAW_SCALE_MIN = 0.65f;
  constexpr float MOTION_ACTIVE_YAW_SCALE_MAX = 1.35f;
  constexpr float MOTION_SLEEPY_YAW_DAMP_MAX = 0.45f;
  constexpr float MOTION_CURIOUS_TILT_DEG_MAX = 6.0f;
  constexpr float MOTION_SLEEPY_TILT_DEG_MAX = 8.0f;
  constexpr float MOTION_ACTIVE_INTERVAL_SCALE = 0.35f;
  constexpr float MOTION_SLEEPY_INTERVAL_SCALE = 0.65f;
  constexpr unsigned long MOTION_INTERVAL_MIN_MS = 800;
  constexpr unsigned long MOTION_INTERVAL_MAX_MS = 3200;
}
namespace Behavior {
  constexpr unsigned long ACTION_MIN_INTERVAL_MS = 220;
  constexpr unsigned long IDLE_EVAL_INTERVAL_MS = 900;

  constexpr unsigned long TOUCH_FACE_HOLD_MS = 900;
  constexpr unsigned long TILT_FACE_HOLD_MS = 800;
  constexpr unsigned long SHAKE_FACE_HOLD_MS = 1100;
  constexpr unsigned long FALL_FACE_HOLD_MS = 1500;
  constexpr unsigned long EMOTION_FACE_HOLD_MS = 650;

  constexpr float EMOTION_LOW_ENERGY = 0.28f;
  constexpr float EMOTION_NEG_VALENCE = -0.35f;
  constexpr float EMOTION_POS_VALENCE = 0.35f;
  constexpr float EMOTION_HIGH_CURIOSITY = 0.65f;
  constexpr float EMOTION_HIGH_AROUSAL = 0.72f;

  constexpr unsigned long LONG_IDLE_MS = 7000;
  constexpr unsigned long AUTONOMY_MIN_IDLE_MS = 2500;
  constexpr unsigned long AUTONOMY_ACTION_COOLDOWN_MS = 2400;
  constexpr unsigned int AUTONOMY_IDLE_TICKS_FOR_SCAN = 2;
  constexpr float AUTONOMY_CURIOSITY_SCAN_MIN = 0.45f;

  constexpr unsigned long SOCIAL_INITIATIVE_INTERVAL_MS = 9000;
  constexpr float SOCIAL_INITIATIVE_ATTENTION_MIN = 0.55f;
  constexpr float SOCIAL_INITIATIVE_BOND_MIN = 0.35f;
}
namespace Power {
#if NCOS_SIM_MODE
  constexpr unsigned long SAMPLE_INTERVAL_MS = 1000;
  constexpr int SIM_BATTERY_PERCENT = 82;
  constexpr bool SIM_CHARGING = false;
#else
  constexpr unsigned long SAMPLE_INTERVAL_MS = 1200;
  constexpr int SIM_BATTERY_PERCENT = 100;
  constexpr bool SIM_CHARGING = false;
#endif

  constexpr int ADC_MAX = 4095;
  constexpr int ADC_REF_MV = 3300;
  constexpr float BATTERY_DIVIDER_RATIO = 2.0f;

  constexpr int BATTERY_EMPTY_MV = 3300;
  constexpr int BATTERY_FULL_MV = 4200;
  constexpr int CHARGE_DETECT_ACTIVE_LEVEL = 0;

  constexpr int LOW_POWER_ENTER_PERCENT = 25;
  constexpr int LOW_POWER_EXIT_PERCENT = 30;
  constexpr int SLEEP_ENTER_PERCENT = 10;
  constexpr int SLEEP_EXIT_PERCENT = 16;

  constexpr unsigned long PUBLISH_STATUS_INTERVAL_MS = 4000;

  constexpr float LOW_POWER_FACE_INTERVAL_SCALE = 1.8f;
  constexpr float LOW_POWER_SENSOR_INTERVAL_SCALE = 1.4f;
  constexpr float LOW_POWER_MOTION_INTERVAL_SCALE = 1.8f;
  constexpr float LOW_POWER_HEARTBEAT_INTERVAL_SCALE = 2.0f;

  constexpr float SLEEP_FACE_INTERVAL_SCALE = 4.0f;
  constexpr float SLEEP_SENSOR_INTERVAL_SCALE = 3.0f;
  constexpr float SLEEP_MOTION_INTERVAL_SCALE = 4.0f;
  constexpr float SLEEP_HEARTBEAT_INTERVAL_SCALE = 5.0f;
}
namespace System {
#if NCOS_SIM_MODE && NCOS_SIM_ULTRA
  constexpr unsigned long HEARTBEAT_INTERVAL_MS = 8000;
  constexpr unsigned long FACE_FRAME_INTERVAL_MS = 40; // 25 FPS
  constexpr unsigned long SENSOR_POLL_INTERVAL_MS = 120;
  constexpr unsigned long MOTION_UPDATE_INTERVAL_MS = 40;
#elif NCOS_SIM_MODE
  constexpr unsigned long HEARTBEAT_INTERVAL_MS = 5000;
  constexpr unsigned long FACE_FRAME_INTERVAL_MS = 66;  // 15 FPS
  constexpr unsigned long SENSOR_POLL_INTERVAL_MS = 100;
  constexpr unsigned long MOTION_UPDATE_INTERVAL_MS = 66;
#else
  constexpr unsigned long HEARTBEAT_INTERVAL_MS = 1000;
  constexpr unsigned long FACE_FRAME_INTERVAL_MS = 33;
  constexpr unsigned long SENSOR_POLL_INTERVAL_MS = 50;
  constexpr unsigned long MOTION_UPDATE_INTERVAL_MS = 20;
#endif
}

} // namespace HardwareConfig












