#include "self_test_service.h"

#include "../../config/feature_flags.h"
#include "../../config/hardware_config.h"
#include "../../models/event.h"
#include "../../models/motion_pose.h"
#include <Arduino.h>

SelfTestService::SelfTestService(EventBus& eventBus,
                                 Diagnostics& diagnostics,
                                 IFaceController& faceController,
                                 MotionHAL& motionHal,
                                 SensorHAL& sensorHal,
                                 StorageManager& storageManager,
                                 IPower& powerService)
    : eventBus_(eventBus),
      diagnostics_(diagnostics),
      faceController_(faceController),
      motionHal_(motionHal),
      sensorHal_(sensorHal),
      storageManager_(storageManager),
      powerService_(powerService) {}

void SelfTestService::init() {
  eventBus_.subscribe(EventType::BootComplete, this);
  eventBus_.subscribe(EventType::FaceFrameRendered, this);
}

void SelfTestService::update(unsigned long nowMs) {
  if (completed_ || !bootSeen_) {
    return;
  }

  if (!checksStarted_ && (nowMs - bootMs_) >= HardwareConfig::Recovery::SELF_TEST_BOOT_SETTLE_MS) {
    checksStarted_ = true;
    checkStartMs_ = nowMs;
    runImmediateChecks();
  }

  if (!checksStarted_) {
    return;
  }

  const bool faceTimedOut = (nowMs - checkStartMs_) >= HardwareConfig::Recovery::SELF_TEST_FACE_FRAME_TIMEOUT_MS;
  if (!faceFrameSeen_ && faceTimedOut) {
    markFailure(FailureBit::DisplayFail);
    markFailure(FailureBit::FaceEngineFail);
  }

  const bool faceCheckComplete = !FeatureFlags::DISPLAY_ENABLED || faceFrameSeen_ || faceTimedOut;
  const bool allChecksDone = motionChecked_ && sensorChecked_ && storageChecked_ && powerChecked_ && faceCheckComplete;

  if (allChecksDone) {
    complete(nowMs);
  }
}

void SelfTestService::onEvent(const Event& event) {
  if (event.type == EventType::BootComplete) {
    bootSeen_ = true;
    bootMs_ = event.timestamp;
    return;
  }

  if (event.type == EventType::FaceFrameRendered) {
    faceFrameSeen_ = true;
  }
}

bool SelfTestService::isComplete() const {
  return completed_;
}

bool SelfTestService::passed() const {
  return completed_ && failureMask_ == 0;
}

uint16_t SelfTestService::failureMask() const {
  return failureMask_;
}

void SelfTestService::runImmediateChecks() {
  if (FeatureFlags::DISPLAY_ENABLED) {
    faceController_.requestExpression(ExpressionType::Neutral, EyeAnimPriority::Critical, 250);
  }

  if (FeatureFlags::SERVO_BUS_ENABLED) {
    const bool motionOk = motionHal_.isReady() && motionHal_.applyPose(MotionPoses::center());
    if (!motionOk) {
      markFailure(FailureBit::NeckMotionFail);
    }
  }
  motionChecked_ = true;

  if (FeatureFlags::IMU_ENABLED || FeatureFlags::TOUCH_ENABLED) {
    SensorSnapshot snapshot;
    if (!sensorHal_.sample(snapshot)) {
      markFailure(FailureBit::SensorFail);
    }
  }
  sensorChecked_ = true;

  if (FeatureFlags::STORAGE_SD_ENABLED && !storageManager_.isReady()) {
    markFailure(FailureBit::StorageFail);
  }
  storageChecked_ = true;

  if (!powerService_.getPowerState().valid()) {
    markFailure(FailureBit::PowerFail);
  }
  powerChecked_ = true;
}

void SelfTestService::complete(unsigned long nowMs) {
  completed_ = true;

  Event event;
  event.type = EventType::EVT_SELF_TEST_RESULT;
  event.source = EventSource::SelfTestService;
  event.value = static_cast<int>(failureMask_);
  event.timestamp = nowMs;
  eventBus_.publish(event);

  if (failureMask_ == 0) {
    diagnostics_.logInfo("[SELF_TEST] passed");
  } else {
    diagnostics_.logError("[SELF_TEST] failed");
  }
}

void SelfTestService::markFailure(FailureBit failure) {
  failureMask_ = static_cast<uint16_t>(failureMask_ | static_cast<uint16_t>(failure));
}
