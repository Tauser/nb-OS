#pragma once

class StorageManager {
public:
  void init();
  bool isReady() const;

private:
  bool ready_ = false;
};
