#pragma once

#include "../../core/config_manager.h"
#include "../../core/diagnostics.h"
#include "../../core/event_bus.h"
#include "../../core/storage_manager.h"
#include "../../interfaces/i_event_listener.h"
#include "../../interfaces/i_power.h"
#include "../../models/ota_state.h"

class OTAService : public IEventListener {
public:
  OTAService(EventBus& eventBus,
             Diagnostics& diagnostics,
             StorageManager& storageManager,
             ConfigManager& configManager,
             IPower& powerService);

  void init();
  void update(unsigned long nowMs);
  void onEvent(const Event& event) override;

  const OtaState& getState() const;

private:
  bool policyAllowsApply() const;
  void evaluateStagedPackage(unsigned long nowMs);
  void startApply(unsigned long nowMs);
  void finishApply(unsigned long nowMs);
  void failApply(int errorCode, unsigned long nowMs);
  void publishStatus(unsigned long nowMs);
  uint32_t makeIntegrityToken(uint32_t version) const;

  EventBus& eventBus_;
  Diagnostics& diagnostics_;
  StorageManager& storageManager_;
  ConfigManager& configManager_;
  IPower& powerService_;

  OtaState state_{};

  bool safeModeActive_ = false;
  bool applyRequested_ = false;
  bool selfTestPassed_ = true;

  unsigned long lastCheckMs_ = 0;
  unsigned long lastStatusPublishMs_ = 0;
  unsigned long applyStartMs_ = 0;
};
