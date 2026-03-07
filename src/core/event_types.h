#pragma once

enum class EventType {
  None = 0,
  Any,
  BootStarted,
  BootComplete,
  Heartbeat,
  FaceFrameRendered,
  CameraFrameSampled
};

enum class EventSource {
  Unknown = 0,
  System,
  FaceService,
  VisionService
};
