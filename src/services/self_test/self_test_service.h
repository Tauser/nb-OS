#pragma once

#include "../../core/diagnostics.h"
#include "../../core/event_bus.h"
#include "../../core/storage_manager.h"
#include "../../hal/motion_hal.h"
#include "../../hal/sensor_hal.h"
#include "../../interfaces/i_event_listener.h"
#include "../../interfaces/i_face_controller.h"
#include "../../interfaces/i_power.h"
#include <stdint.h>

class SelfTestService : public IEventListener {
public:
  enum FailureBit : uint16_t {
    None = 0,
    DisplayFail = 1 << 0,
    FaceEngineFail = 1 << 1,
    NeckMotionFail = 1 << 2,
    SensorFail = 1 << 3,
    StorageFail = 1 << 4,
    PowerFail = 1 << 5
  };

  SelfTestService(EventBus& eventBus,
                  Diagnostics& diagnostics,
                  IFaceController& faceController,
                  MotionHAL& motionHal,
                  SensorHAL& sensorHal,
                  StorageManager& storageManager,
                  IPower& powerService);

  void init();
  void update(unsigned long nowMs);
  void onEvent(const Event& event) override;

  bool isComplete() const;
  bool passed() const;
  uint16_t failureMask() const;

private:
  void runImmediateChecks();
  void complete(unsigned long nowMs);
  void markFailure(FailureBit failure);

  EventBus& eventBus_;
  Diagnostics& diagnostics_;
  IFaceController& faceController_;
  MotionHAL& motionHal_;
  SensorHAL& sensorHal_;
  StorageManager& storageManager_;
  IPower& powerService_;

  bool bootSeen_ = false;
  bool checksStarted_ = false;
  bool completed_ = false;
  bool faceFrameSeen_ = false;

  bool motionChecked_ = false;
  bool sensorChecked_ = false;
  bool storageChecked_ = false;
  bool powerChecked_ = false;

  unsigned long bootMs_ = 0;
  unsigned long checkStartMs_ = 0;
  uint16_t failureMask_ = 0;
};
