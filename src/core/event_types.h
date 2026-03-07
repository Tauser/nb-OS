#pragma once

enum class EventType {
  None = 0,
  Any,
  BootStarted,
  BootComplete,
  Heartbeat,
  FaceFrameRendered,
  CameraFrameSampled,
  EVT_TOUCH,
  EVT_SHAKE,
  EVT_TILT,
  EVT_FALL
};

enum class EventSource {
  Unknown = 0,
  System,
  FaceService,
  VisionService,
  SensorService,
  InteractionService
};
