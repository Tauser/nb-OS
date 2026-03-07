#pragma once

class ISensorHub {
public:
  virtual ~ISensorHub() = default;
  virtual void init() = 0;
  virtual void update(unsigned long nowMs) = 0;
};
