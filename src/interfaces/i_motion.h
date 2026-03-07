#pragma once

class IMotion {
public:
  virtual ~IMotion() = default;
  virtual void center() = 0;
};
