#include "storage_manager.h"

void StorageManager::init() {
  ready_ = true;
}

bool StorageManager::isReady() const {
  return ready_;
}
