#pragma once

class IAudioIn {
public:
  virtual ~IAudioIn() = default;
  virtual bool isReady() const = 0;
};
