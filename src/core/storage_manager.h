#pragma once

#include <stdint.h>

class StorageManager {
public:
  void init();
  bool isReady() const;

  bool stageOtaPackage(uint32_t targetVersion, uint32_t checksum);
  bool getStagedOtaPackage(uint32_t& targetVersion, uint32_t& checksum) const;
  void clearStagedOtaPackage();

private:
  bool ready_ = false;
  bool otaPackageStaged_ = false;
  uint32_t stagedVersion_ = 0;
  uint32_t stagedChecksum_ = 0;
};
