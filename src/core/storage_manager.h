#pragma once

#include "../models/fault_record.h"
#include "../models/long_term_memory.h"
#include "../models/rollback_marker.h"
#include <stdint.h>

class StorageManager {
public:
  void init();
  bool isReady() const;

  bool stageOtaPackage(uint32_t targetVersion, uint32_t checksum);
  bool getStagedOtaPackage(uint32_t& targetVersion, uint32_t& checksum) const;
  void clearStagedOtaPackage();

  bool saveRollbackMarker(const RollbackMarker& marker);
  bool loadRollbackMarker(RollbackMarker& outMarker) const;
  void clearRollbackMarker();

  bool appendFaultRecord(const FaultRecord& record);
  bool loadFaultHistory(FaultHistory& outHistory) const;
  void clearFaultHistory();

  bool loadLongTermMemory(LongTermMemory& outMemory) const;
  bool saveLongTermMemory(const LongTermMemory& memory);

  uint32_t bootCount() const;

private:
  bool ready_ = false;
  bool otaPackageStaged_ = false;
  uint32_t stagedVersion_ = 0;
  uint32_t stagedChecksum_ = 0;
  uint32_t bootCount_ = 0;
};
