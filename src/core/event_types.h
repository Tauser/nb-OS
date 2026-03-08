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
  EVT_FALL,
  EVT_IDLE,
  EVT_VOICE_ACTIVITY,
  EVT_MOTION_POSE_APPLIED,
  EVT_EMOTION_CHANGED,
  EVT_BEHAVIOR_ACTION
};

enum class EventSource {
  Unknown = 0,
  System,
  FaceService,
  VisionService,
  SensorService,
  MotionService,
  EmotionService,
  InteractionService,
  BehaviorService
};
