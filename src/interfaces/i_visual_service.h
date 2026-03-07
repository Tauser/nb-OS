#pragma once

class IVisualService {
public:
  virtual ~IVisualService() = default;
  virtual void init() = 0;
  virtual void update(unsigned long nowMs) = 0;
};
