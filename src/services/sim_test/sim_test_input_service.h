#pragma once

#include <stddef.h>

#include "../../core/event_bus.h"
#include "../../interfaces/i_face_controller.h"

class SimTestInputService {
public:
  SimTestInputService(EventBus& eventBus, IFaceController& faceController);

  void init();
  void update(unsigned long nowMs);

private:
  void processCommand(char* line, unsigned long nowMs);
  void publish(EventType type, unsigned long nowMs, int value = 0);
  void printHelp() const;

  EventBus& eventBus_;
  IFaceController& faceController_;

  static constexpr size_t kMaxCmdLen = 64;
  char cmdBuffer_[kMaxCmdLen] = {0};
  size_t cmdLen_ = 0;
};
