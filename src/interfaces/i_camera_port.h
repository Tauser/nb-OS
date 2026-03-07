#pragma once

class ICameraPort {
public:
  virtual ~ICameraPort() = default;

  virtual bool init() = 0;
  virtual bool sampleFrame() = 0;
};
