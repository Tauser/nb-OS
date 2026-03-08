#include "storage_manager.h"

#include <Preferences.h>

namespace {
constexpr const char* kNs = "ncos";
constexpr const char* kMemBlobKey = "ltm_blob";
constexpr uint32_t kLtmSchema = 2;
}

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

bool StorageManager::loadLongTermMemory(LongTermMemory& outMemory) const {
  if (!ready_) {
    return false;
  }

  Preferences prefs;
  if (!prefs.begin(kNs, true)) {
    return false;
  }

  LongTermMemory loaded{};
  const size_t len = prefs.getBytes(kMemBlobKey, &loaded, sizeof(loaded));
  prefs.end();

  if (len != sizeof(loaded)) {
    return false;
  }

  if (loaded.schemaVersion != kLtmSchema) {
    return false;
  }

  loaded.sanitize();
  outMemory = loaded;
  return true;
}

bool StorageManager::saveLongTermMemory(const LongTermMemory& memory) {
  if (!ready_) {
    return false;
  }

  LongTermMemory toSave = memory;
  toSave.schemaVersion = kLtmSchema;
  toSave.sanitize();

  Preferences prefs;
  if (!prefs.begin(kNs, false)) {
    return false;
  }

  const size_t written = prefs.putBytes(kMemBlobKey, &toSave, sizeof(toSave));
  prefs.end();

  return written == sizeof(toSave);
}

