#include "storage_manager.h"

void StorageManager::init() {
  ready_ = true;
}

bool StorageManager::isReady() const {
  return ready_;
}

bool StorageManager::stageOtaPackage(uint32_t targetVersion, uint32_t checksum) {
  if (!ready_ || targetVersion == 0 || checksum == 0) {
    return false;
  }

  otaPackageStaged_ = true;
  stagedVersion_ = targetVersion;
  stagedChecksum_ = checksum;
  return true;
}

bool StorageManager::getStagedOtaPackage(uint32_t& targetVersion, uint32_t& checksum) const {
  if (!ready_ || !otaPackageStaged_) {
    return false;
  }

  targetVersion = stagedVersion_;
  checksum = stagedChecksum_;
  return true;
}

void StorageManager::clearStagedOtaPackage() {
  otaPackageStaged_ = false;
  stagedVersion_ = 0;
  stagedChecksum_ = 0;
}
