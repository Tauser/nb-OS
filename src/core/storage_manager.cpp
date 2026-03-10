#include "storage_manager.h"

#include <Preferences.h>

namespace {
constexpr const char* kNs = "ncos";
constexpr const char* kMemBlobKey = "ltm_blob";
constexpr const char* kOtaBlobKey = "ota_pkg";
constexpr const char* kRollbackBlobKey = "ota_rb";
constexpr const char* kFaultBlobKey = "fault_hist";
constexpr const char* kBootCountKey = "boot_count";
constexpr uint32_t kLtmSchema = 2;

struct OtaPackageBlob {
  uint8_t staged = 0;
  uint8_t reserved0 = 0;
  uint16_t reserved1 = 0;
  uint32_t targetVersion = 0;
  uint32_t checksum = 0;
};
}

void StorageManager::init() {
  ready_ = true;

  Preferences prefs;
  if (!prefs.begin(kNs, false)) {
    ready_ = false;
    return;
  }

  OtaPackageBlob pkg{};
  const size_t pkgLen = prefs.getBytes(kOtaBlobKey, &pkg, sizeof(pkg));
  if (pkgLen == sizeof(pkg) && pkg.staged != 0 && pkg.targetVersion != 0 && pkg.checksum != 0) {
    otaPackageStaged_ = true;
    stagedVersion_ = pkg.targetVersion;
    stagedChecksum_ = pkg.checksum;
  } else {
    otaPackageStaged_ = false;
    stagedVersion_ = 0;
    stagedChecksum_ = 0;
  }

  const uint32_t prevBoots = prefs.getUInt(kBootCountKey, 0U);
  bootCount_ = prevBoots + 1U;
  prefs.putUInt(kBootCountKey, bootCount_);

  prefs.end();
}

bool StorageManager::isReady() const {
  return ready_;
}

bool StorageManager::stageOtaPackage(uint32_t targetVersion, uint32_t checksum) {
  if (!ready_ || targetVersion == 0 || checksum == 0) {
    return false;
  }

  Preferences prefs;
  if (!prefs.begin(kNs, false)) {
    return false;
  }

  OtaPackageBlob pkg{};
  pkg.staged = 1;
  pkg.targetVersion = targetVersion;
  pkg.checksum = checksum;
  const size_t written = prefs.putBytes(kOtaBlobKey, &pkg, sizeof(pkg));
  prefs.end();

  if (written != sizeof(pkg)) {
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

  if (!ready_) {
    return;
  }

  Preferences prefs;
  if (!prefs.begin(kNs, false)) {
    return;
  }

  OtaPackageBlob pkg{};
  prefs.putBytes(kOtaBlobKey, &pkg, sizeof(pkg));
  prefs.end();
}

bool StorageManager::saveRollbackMarker(const RollbackMarker& marker) {
  if (!ready_) {
    return false;
  }

  RollbackMarker toSave = marker;
  toSave.sanitize();

  Preferences prefs;
  if (!prefs.begin(kNs, false)) {
    return false;
  }

  const size_t written = prefs.putBytes(kRollbackBlobKey, &toSave, sizeof(toSave));
  prefs.end();
  return written == sizeof(toSave);
}

bool StorageManager::loadRollbackMarker(RollbackMarker& outMarker) const {
  if (!ready_) {
    return false;
  }

  Preferences prefs;
  if (!prefs.begin(kNs, true)) {
    return false;
  }

  RollbackMarker marker{};
  const size_t len = prefs.getBytes(kRollbackBlobKey, &marker, sizeof(marker));
  prefs.end();

  if (len != sizeof(marker)) {
    outMarker = RollbackMarker{};
    return false;
  }

  marker.sanitize();
  outMarker = marker;
  return true;
}

void StorageManager::clearRollbackMarker() {
  if (!ready_) {
    return;
  }

  RollbackMarker marker{};
  saveRollbackMarker(marker);
}

bool StorageManager::appendFaultRecord(const FaultRecord& record) {
  if (!ready_) {
    return false;
  }

  FaultHistory history{};
  loadFaultHistory(history);

  FaultRecord toStore = record;
  if (toStore.bootCount == 0) {
    toStore.bootCount = bootCount_;
  }
  history.push(toStore);

  Preferences prefs;
  if (!prefs.begin(kNs, false)) {
    return false;
  }

  const size_t written = prefs.putBytes(kFaultBlobKey, &history, sizeof(history));
  prefs.end();
  return written == sizeof(history);
}

bool StorageManager::loadFaultHistory(FaultHistory& outHistory) const {
  if (!ready_) {
    return false;
  }

  Preferences prefs;
  if (!prefs.begin(kNs, true)) {
    return false;
  }

  FaultHistory loaded{};
  const size_t len = prefs.getBytes(kFaultBlobKey, &loaded, sizeof(loaded));
  prefs.end();

  if (len != sizeof(loaded)) {
    outHistory = FaultHistory{};
    return false;
  }

  loaded.sanitize();
  outHistory = loaded;
  return true;
}

void StorageManager::clearFaultHistory() {
  if (!ready_) {
    return;
  }

  FaultHistory empty{};
  Preferences prefs;
  if (!prefs.begin(kNs, false)) {
    return;
  }
  prefs.putBytes(kFaultBlobKey, &empty, sizeof(empty));
  prefs.end();
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

uint32_t StorageManager::bootCount() const {
  return bootCount_;
}
