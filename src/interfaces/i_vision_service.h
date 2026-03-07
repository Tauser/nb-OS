#pragma once

class IVisionService {
public:
  virtual ~IVisionService() = default;
  virtual void init() = 0;
  virtual void update() = 0;
};
