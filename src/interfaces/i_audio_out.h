#pragma once

class IAudioOut {
public:
  virtual ~IAudioOut() = default;
  virtual bool isReady() const = 0;
};
