#pragma once

#include "../../core/diagnostics.h"
#include "../../core/event_bus.h"
#include "../../core/storage_manager.h"
#include "../../interfaces/i_event_listener.h"
#include "../../interfaces/i_motion.h"

class SafeModeService : public IEventListener {
public:
  enum DegradedSubsystem : uint16_t {
    DegradeNone = 0,
    DegradeDisplay = 1 << 0,
    DegradeFace = 1 << 1,
    DegradeMotion = 1 << 2,
    DegradeSensor = 1 << 3,
    DegradeStorage = 1 << 4,
    DegradePower = 1 << 5,
    DegradeVision = 1 << 6,
    DegradeVoice = 1 << 7,
    DegradeCloud = 1 << 8,
    DegradeOta = 1 << 9
  };

  SafeModeService(EventBus& eventBus, Diagnostics& diagnostics, StorageManager& storageManager, IMotion& motion);

  void init();
  void update(unsigned long nowMs);
  void onEvent(const Event& event) override;

  bool isActive() const;
  uint16_t degradedMask() const;

private:
  void enterSafeMode(unsigned long nowMs, uint16_t degradedMask, uint16_t reasonCode, const char* reason);
  uint16_t degradedMaskFromSelfTest(uint16_t failureMask) const;
  uint16_t degradedMaskFromHealthAnomaly(int anomalyCode) const;

  EventBus& eventBus_;
  Diagnostics& diagnostics_;
  StorageManager& storageManager_;
  IMotion& motion_;

  bool active_ = false;
  uint16_t degradedMask_ = DegradeNone;
};
