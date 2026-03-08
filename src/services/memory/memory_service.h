#pragma once

#include "../../core/event_bus.h"
#include "../../core/storage_manager.h"
#include "../../interfaces/i_event_listener.h"
#include "../../models/memory_profile.h"

class MemoryService : public IEventListener {
public:
  MemoryService(EventBus& eventBus, StorageManager& storageManager);

  void init();
  void update(unsigned long nowMs);
  void onEvent(const Event& event) override;

  const MemoryProfile& getProfile() const;

private:
  void rebuildPreference();
  void mergeSessionIntoLongTerm(unsigned long nowMs);
  void publishPreference(unsigned long nowMs);
  void publishMemoryStatus(unsigned long nowMs);

  EventBus& eventBus_;
  StorageManager& storageManager_;

  MemoryProfile profile_{};
  AttentionFocus lastPublishedFocus_ = AttentionFocus::None;
  bool lastPublishedCalmStyle_ = false;
  bool dirtyLongTerm_ = false;

  unsigned long lastUpdateMs_ = 0;
  unsigned long lastPersistMs_ = 0;
  unsigned long lastSummaryMs_ = 0;
  uint32_t lastMergedSessionDurationMs_ = 0;
};
