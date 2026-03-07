#pragma once

class ISensorHub {
public:
  virtual ~ISensorHub() = default;
  virtual void update() = 0;
};
