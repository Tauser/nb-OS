#include "motion_sync_service.h"

#include "../../config/hardware_config.h"
#include "../../models/event.h"
#include "../../utils/math_utils.h"
#include <Arduino.h>

namespace {
float clampf(float value, float minValue, float maxValue) {
  return MathUtils::clamp(value, minValue, maxValue);
}

float absf(float value) {
  return value < 0.0f ? -value : value;
}

MotionPose makePose(MotionPoseId id, float yawDeg, float tiltDeg, unsigned long durationMs) {
  MotionPose pose;
  pose.id = id;
  pose.yawDeg = yawDeg;
  pose.tiltDeg = tiltDeg;
  pose.durationMs = durationMs;
  return pose;
}

FaceSyncCue clipCueFor(FaceClipKind kind) {
  switch (kind) {
    case FaceClipKind::WakeUp: return FaceSyncCue::ClipWakeUp;
    case FaceClipKind::GoToSleep: return FaceSyncCue::ClipGoToSleep;
    case FaceClipKind::AttentionRecovery: return FaceSyncCue::ClipAttentionRecovery;
    case FaceClipKind::ThinkingLoop: return FaceSyncCue::ClipThinkingLoop;
    case FaceClipKind::SoftListen: return FaceSyncCue::ClipSoftListen;
    case FaceClipKind::ShyRetract: return FaceSyncCue::ClipShyRetract;
    case FaceClipKind::HappyAck: return FaceSyncCue::ClipHappyAck;
    case FaceClipKind::None:
    default:
      return FaceSyncCue::None;
  }
}
}

MotionSyncService::MotionSyncService(EventBus& eventBus,
                                     const IFaceRenderStateProvider& faceRenderStateProvider,
                                     const IMotionStateProvider& motionStateProvider,
                                     IFaceController& faceController,
                                     IMotion& motion)
    : eventBus_(eventBus),
      faceRenderStateProvider_(faceRenderStateProvider),
      motionStateProvider_(motionStateProvider),
      faceController_(faceController),
      motion_(motion) {}

void MotionSyncService::init() {
  eventBus_.subscribe(EventType::EVT_VOICE_START, this);
  eventBus_.subscribe(EventType::EVT_VOICE_END, this);
  eventBus_.subscribe(EventType::EVT_EMOTION_CHANGED, this);

  const unsigned long nowMs = millis();
  lastSyncMs_ = nowMs;
  lastGazeSyncMs_ = nowMs;
  lastClipSyncMs_ = nowMs;
  lastCueMs_ = nowMs;
  lastCenterRecoveryMs_ = nowMs;
}

void MotionSyncService::update(unsigned long nowMs) {
  const FaceRenderState& faceState = faceRenderStateProvider_.getFaceRenderState();
  syncClipWithMotion(faceState, nowMs);
  syncGazeWithMotion(faceState, nowMs);
}

void MotionSyncService::onEvent(const Event& event) {
  const unsigned long nowMs = (event.timestamp > 0) ? event.timestamp : millis();
  if (nowMs - lastSyncMs_ < HardwareConfig::Polish::MOTION_SYNC_COOLDOWN_MS) {
    return;
  }

  switch (event.type) {
    case EventType::EVT_VOICE_START:
      faceController_.requestExpression(ExpressionType::Listening, EyeAnimPriority::Social, 520);
      motion_.softListen();
      publishCue(FaceSyncCue::ClipSoftListen, nowMs);
      lastSyncMs_ = nowMs;
      break;

    case EventType::EVT_VOICE_END:
      motion_.idleSway();
      lastSyncMs_ = nowMs;
      break;

    case EventType::EVT_EMOTION_CHANGED: {
      const float arousal = event.hasTypedPayload(EventPayloadKind::EmotionChanged)
                                ? event.payload.emotionChanged.arousal
                                : (static_cast<float>(event.value) / 1000.0f);
      if (arousal > 0.70f) {
        if (yawLeft_) {
          motion_.yawLeft();
          publishCue(FaceSyncCue::GazeLeft, nowMs);
        } else {
          motion_.yawRight();
          publishCue(FaceSyncCue::GazeRight, nowMs);
        }
        yawLeft_ = !yawLeft_;
        lastSyncMs_ = nowMs;
      }
      break;
    }

    default:
      break;
  }
}

void MotionSyncService::syncClipWithMotion(const FaceRenderState& faceState, unsigned long nowMs) {
  const FaceClipState& clip = faceState.clip;

  if (clip.active && clip.kind != FaceClipKind::None) {
    const bool changedClip = !lastClipActive_ || (clip.kind != lastClipKind_);
    if (changedClip && (nowMs - lastClipSyncMs_ >= HardwareConfig::MotionSync::CLIP_MOTION_COOLDOWN_MS)) {
      switch (clip.kind) {
        case FaceClipKind::WakeUp:
          motion_.center();
          break;

        case FaceClipKind::GoToSleep:
          motion_.softListen();
          break;

        case FaceClipKind::AttentionRecovery:
          motion_.idleSway();
          break;

        case FaceClipKind::ThinkingLoop: {
          const float yaw = yawLeft_ ? -6.0f : 6.0f;
          yawLeft_ = !yawLeft_;
          motion_.applyPose(makePose(MotionPoseId::SoftListen, yaw, 10.0f, 260));
          break;
        }

        case FaceClipKind::SoftListen:
          motion_.softListen();
          break;

        case FaceClipKind::ShyRetract:
          if (yawLeft_) {
            motion_.yawLeft();
          } else {
            motion_.yawRight();
          }
          yawLeft_ = !yawLeft_;
          break;

        case FaceClipKind::HappyAck:
          if (yawLeft_) {
            motion_.tiltLeft();
          } else {
            motion_.tiltRight();
          }
          yawLeft_ = !yawLeft_;
          break;

        case FaceClipKind::None:
        default:
          break;
      }

      publishCue(clipCueFor(clip.kind), nowMs);
      lastClipSyncMs_ = nowMs;
    }
  } else if (lastClipActive_) {
    publishCue(FaceSyncCue::ClipEnded, nowMs);
  }

  lastClipActive_ = clip.active;
  lastClipKind_ = clip.kind;
}

void MotionSyncService::syncGazeWithMotion(const FaceRenderState& faceState, unsigned long nowMs) {
  if (nowMs - lastGazeSyncMs_ < HardwareConfig::MotionSync::GAZE_SYNC_INTERVAL_MS) {
    return;
  }

  if (faceState.clip.active && faceState.clip.ownsGaze) {
    return;
  }

  const float currentYaw = motionStateProvider_.getCurrentYawDeg();
  const float currentTilt = motionStateProvider_.getCurrentTiltDeg();

  if (!faceState.gaze.enabled) {
    if (absf(currentYaw) >= HardwareConfig::MotionSync::GAZE_CENTER_YAW_THRESHOLD_DEG &&
        nowMs - lastCenterRecoveryMs_ >= HardwareConfig::MotionSync::GAZE_CENTER_RECOVERY_MS) {
      const float gentleTilt = clampf(currentTilt * 0.65f,
                                      HardwareConfig::Motion::TILT_MIN_DEG,
                                      HardwareConfig::Motion::TILT_MAX_DEG);
      motion_.applyPose(makePose(MotionPoseId::Center,
                                 0.0f,
                                 gentleTilt,
                                 HardwareConfig::MotionSync::GAZE_POSE_DURATION_MS + 90));
      publishCue(FaceSyncCue::GazeCenter, nowMs);
      lastCenterRecoveryMs_ = nowMs;
    }

    lastGazeTargetXNorm_ = 0.0f;
    lastGazeSyncMs_ = nowMs;
    return;
  }

  const float targetX = clampf(faceState.gaze.targetXNorm, -1.0f, 1.0f);
  const float targetY = clampf(faceState.gaze.targetYNorm, -1.0f, 1.0f);
  const float targetDelta = absf(targetX - lastGazeTargetXNorm_);

  const float desiredYaw = clampf(targetX * HardwareConfig::Motion::YAW_LOOK_DEG * HardwareConfig::MotionSync::GAZE_YAW_SCALE,
                                  HardwareConfig::Motion::YAW_MIN_DEG,
                                  HardwareConfig::Motion::YAW_MAX_DEG);
  const float desiredTilt = clampf((-targetY) * HardwareConfig::Motion::TILT_LOOK_DEG * HardwareConfig::MotionSync::GAZE_TILT_SCALE,
                                   HardwareConfig::Motion::TILT_MIN_DEG,
                                   HardwareConfig::Motion::TILT_MAX_DEG);

  const float yawDelta = absf(desiredYaw - currentYaw);
  if (targetDelta < HardwareConfig::MotionSync::GAZE_MIN_DELTA_NORM &&
      yawDelta < HardwareConfig::MotionSync::GAZE_MIN_YAW_DELTA_DEG) {
    lastGazeSyncMs_ = nowMs;
    return;
  }

  motion_.applyPose(makePose(MotionPoseId::IdleSway,
                             desiredYaw,
                             desiredTilt,
                             HardwareConfig::MotionSync::GAZE_POSE_DURATION_MS));

  if (targetX > 0.16f && lastCueDirectionXNorm_ <= 0.16f) {
    publishCue(FaceSyncCue::GazeRight, nowMs);
  } else if (targetX < -0.16f && lastCueDirectionXNorm_ >= -0.16f) {
    publishCue(FaceSyncCue::GazeLeft, nowMs);
  } else if (absf(targetX) <= 0.08f && absf(lastCueDirectionXNorm_) > 0.08f) {
    publishCue(FaceSyncCue::GazeCenter, nowMs);
  }

  lastCueDirectionXNorm_ = targetX;
  lastGazeTargetXNorm_ = targetX;
  lastGazeSyncMs_ = nowMs;
}

void MotionSyncService::publishCue(FaceSyncCue cue, unsigned long nowMs) {
  if (cue == FaceSyncCue::None) {
    return;
  }

  if (cue != FaceSyncCue::ClipEnded &&
      nowMs - lastCueMs_ < HardwareConfig::MotionSync::CUE_COOLDOWN_MS) {
    return;
  }

  Event event;
  event.type = EventType::EVT_FACE_SYNC_CUE;
  event.source = EventSource::MotionSyncService;
  event.value = static_cast<int>(cue);
  event.timestamp = nowMs;
  eventBus_.publish(event);

  lastCueMs_ = nowMs;
}
