#pragma once

#ifndef NCOS_SIM_MODE
#define NCOS_SIM_MODE 0
#endif

namespace FeatureFlags {
constexpr bool DISPLAY_ENABLED = true;

#if NCOS_SIM_MODE
constexpr bool STORAGE_SD_ENABLED = false;
constexpr bool TOUCH_ENABLED = false;
constexpr bool IMU_ENABLED = false;
constexpr bool AUDIO_INPUT_ENABLED = false;
constexpr bool AUDIO_OUTPUT_ENABLED = false;
constexpr bool SERVO_BUS_ENABLED = false;
constexpr bool CAMERA_ENABLED = false;
#else
constexpr bool STORAGE_SD_ENABLED = true;
constexpr bool TOUCH_ENABLED = true;
constexpr bool IMU_ENABLED = true;
constexpr bool AUDIO_INPUT_ENABLED = true;
constexpr bool AUDIO_OUTPUT_ENABLED = true;
constexpr bool SERVO_BUS_ENABLED = true;
constexpr bool CAMERA_ENABLED = true;
#endif
}