#pragma once

#ifndef NCOS_SIM_MODE
#define NCOS_SIM_MODE 0
#endif

#ifndef FF_TEST_GATES_ENFORCED
#define FF_TEST_GATES_ENFORCED 0
#endif

#ifndef FF_ACTION_ORCHESTRATOR
#define FF_ACTION_ORCHESTRATOR 1
#endif

#ifndef FF_EVENTBUS_V2
#define FF_EVENTBUS_V2 1
#endif

#ifndef FF_COMPANION_AGGREGATE
#define FF_COMPANION_AGGREGATE 1
#endif

#ifndef FF_VISION_REAL
#define FF_VISION_REAL 1
#endif

#ifndef FF_CLOUD_REAL
#define FF_CLOUD_REAL 1
#endif

#ifndef FF_ROBUST_RECOVERY
#define FF_ROBUST_RECOVERY 1
#endif

namespace FeatureFlags {
constexpr bool DISPLAY_ENABLED = true;
constexpr bool LED_ENABLED = true;
constexpr bool TEST_GATES_ENFORCED = (FF_TEST_GATES_ENFORCED != 0);
constexpr bool ACTION_ORCHESTRATOR_ENABLED = (FF_ACTION_ORCHESTRATOR != 0);
constexpr bool EVENTBUS_V2_ENABLED = (FF_EVENTBUS_V2 != 0);
constexpr bool COMPANION_AGGREGATE_ENABLED = (FF_COMPANION_AGGREGATE != 0);
constexpr bool VISION_REAL_ENABLED = (FF_VISION_REAL != 0) && !NCOS_SIM_MODE;
constexpr bool CLOUD_REAL_ENABLED = (FF_CLOUD_REAL != 0);
constexpr bool ROBUST_RECOVERY_ENABLED = (FF_ROBUST_RECOVERY != 0);

#if NCOS_SIM_MODE
constexpr bool STORAGE_SD_ENABLED = false;
constexpr bool TOUCH_ENABLED = false;
constexpr bool IMU_ENABLED = false;
constexpr bool AUDIO_INPUT_ENABLED = false;
constexpr bool AUDIO_OUTPUT_ENABLED = false;
constexpr bool SERVO_BUS_ENABLED = false;
constexpr bool CAMERA_ENABLED = false;
constexpr bool CLOUD_ENABLED = false;
#else
constexpr bool STORAGE_SD_ENABLED = true;
constexpr bool TOUCH_ENABLED = true;
constexpr bool IMU_ENABLED = true;
constexpr bool AUDIO_INPUT_ENABLED = true;
constexpr bool AUDIO_OUTPUT_ENABLED = true;
constexpr bool SERVO_BUS_ENABLED = true;
constexpr bool CAMERA_ENABLED = true;
constexpr bool CLOUD_ENABLED = true;
#endif
} // namespace FeatureFlags
