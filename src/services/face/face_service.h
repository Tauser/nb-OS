#pragma once

#include "../../interfaces/i_display_port.h"
#include "../../interfaces/i_emotion_provider.h"
#include "../../interfaces/i_face_controller.h"
#include "../../interfaces/i_face_render_state_provider.h"
#include "../../interfaces/i_motion_state_provider.h"
#include "../../interfaces/i_visual_service.h"
#include "../../models/motion_pose.h"
#include "expression_preset.h"
#include "face_clip_player.h"
#include "face_compositor.h"
#include "face_renderer.h"
#include "face_gaze_controller.h"
#include "eye_model.h"

class FaceService : public IVisualService, public IFaceController, public IFaceRenderStateProvider {
public:
  FaceService(IDisplayPort& displayPort, const IEmotionProvider& emotionProvider);
  void setMotionStateProvider(const IMotionStateProvider* motionStateProvider);

  void init() override;
  void update(unsigned long nowMs) override;

  void requestExpression(ExpressionType expression, EyeAnimPriority priority, unsigned long holdMs = 0) override;
  void requestGazeTarget(const FaceGazeTarget& target) override;
  void clearGazeTarget() override;
  bool requestClip(FaceClipKind kind, bool allowPreempt = true) override;
  bool cancelClip() override;

  bool tunerSetEnabled(bool enabled) override;
  bool tunerSetPreset(const char* presetName) override;
  bool tunerSetParam(const char* key, float value) override;
  bool tunerReset() override;
  bool tunerGetStatus(char* out, size_t outSize) const override;
  const FaceRenderState& getFaceRenderState() const override;

private:
  enum class BlinkType : uint8_t {
    Normal = 0,
    Short,
    DoubleRare,
    Sleepy,
    Attentive
  };

  enum class IdleMicroAnim : uint8_t {
    None = 0,
    TinySquish,
    SlightRelax,
    SleepySettling,
    CuriosityPulse,
    WakeCorrection
  };

  struct PerfStats {
    unsigned long windowStartMs = 0;
    unsigned long updates = 0;
    unsigned long renderedFrames = 0;
    unsigned long droppedFrames = 0;
    unsigned long maxRenderUs = 0;
    unsigned long avgRenderUsAccum = 0;
  };

  struct FaceLayerState {
    int leftX = 0;
    int rightX = 0;
    int y = 0;
    float openness = 1.0f;
    float upperLid = 0.08f;
    float lowerLid = 0.02f;
    float tiltDeg = 0.0f;
    float squashY = 1.0f;
    float stretchX = 1.0f;
    float roundness = 0.52f;
    float spacingScale = 1.0f;
    float yawNorm = 0.0f;
    float tiltNorm = 0.0f;
    MotionPoseId neckPoseId = MotionPoseId::Center;
  };

  struct FaceLayerDelta {
    int leftX = 0;
    int rightX = 0;
    int y = 0;
    float openness = 0.0f;
    float upperLid = 0.0f;
    float lowerLid = 0.0f;
    float tiltDeg = 0.0f;
    float squashY = 0.0f;
    float stretchX = 0.0f;
    float roundness = 0.0f;
    float spacingScale = 0.0f;
    float yawNorm = 0.0f;
    float tiltNorm = 0.0f;
  };

  void applyEmotionOutput(unsigned long nowMs);

  FaceLayerState buildBaseExpressionLayer(unsigned long nowMs, const EmotionState& emo);
  void applyMoodModulationLayer(FaceLayerState& layer, const EmotionState& emo);
  void applyAttentionModulationLayer(FaceLayerState& layer, const EmotionState& emo);
  void applyNeckCoherenceLayer(FaceLayerState& layer, const EmotionState& emo);
  void applyGazeControllerLayer(FaceLayerState& layer, unsigned long nowMs, const EmotionState& emo);
  void applyIdleModulationLayer(FaceLayerState& layer, unsigned long nowMs, const EmotionState& emo);
  void applyTransientReactionLayer(FaceLayerState& layer, unsigned long nowMs, const EmotionState& emo);
  void applyClipLayer(FaceLayerState& layer);
  void applyBlinkLayer(FaceLayerState& layer, unsigned long nowMs, const EmotionState& emo);
  void commitLayerState(const FaceLayerState& layer);
  FaceLayerDelta captureDelta(const FaceLayerState& before, const FaceLayerState& after) const;
  void applyDelta(FaceLayerState& layer, const FaceLayerDelta& delta) const;
  FaceLayerState resolveLayerWithCompositor(const FaceLayerState& baseLayer,
                                            const FaceLayerDelta& gazeDelta,
                                            const FaceLayerDelta& idleDelta,
                                            const FaceLayerDelta& transientDelta,
                                            const FaceLayerDelta& clipDelta,
                                            const FaceLayerDelta& blinkDelta) const;

  void syncRenderState(unsigned long nowMs, const EmotionState& emo, const FaceLayerState& layer);
  FaceMoodEnvelope chooseMoodEnvelope(const EmotionState& emo) const;
  FaceTransientReactionKind chooseTransientKind() const;
  FaceIdleMode chooseIdleMode() const;

  void startBlink(unsigned long nowMs, const FaceLayerState& baseLayer);
  float computeBlinkPulse(unsigned long elapsedMs) const;
  float computeSingleBlinkPulse(float t) const;
  BlinkType chooseBlinkType(const EmotionState& emo) const;

  void startIdleMicroAnim(unsigned long nowMs, const EmotionState& emo);
  float idleMicroEnvelope(unsigned long nowMs) const;

  void scheduleNextBlink(unsigned long nowMs);
  void stepStateMachine(unsigned long nowMs);
  void queueClipForExpressionTransition(ExpressionType previous, ExpressionType current, unsigned long nowMs);
  void smoothEyes(unsigned long nowMs);
  void beginPresetBlend(FacePresetId from, FacePresetId to, unsigned long nowMs);
  float computePresetBlend(unsigned long nowMs) const;
  float easedBlend(float t, const TransitionProfile& transition) const;
  void recordPerf(unsigned long nowMs, bool rendered, unsigned long renderUs);
  float easeInOutCubic(float t) const;

  IDisplayPort& displayPort_;
  const IEmotionProvider& emotionProvider_;
  const IMotionStateProvider* motionStateProvider_ = nullptr;
  FaceRenderer renderer_;
  FaceGazeController gazeController_;
  FaceClipPlayer clipPlayer_;
  FaceCompositor compositor_;

  EyeModel leftEye_;
  EyeModel rightEye_;
  EyeModel targetLeftEye_;
  EyeModel targetRightEye_;

  ExpressionType currentExpression_ = ExpressionType::Neutral;
  ExpressionType requestedExpression_ = ExpressionType::Neutral;
  EyeAnimPriority currentPriority_ = EyeAnimPriority::Idle;
  EyeAnimPriority requestedPriority_ = EyeAnimPriority::Idle;
  unsigned long expressionHoldUntilMs_ = 0;
  unsigned long autoHoldUntilMs_ = 0;

  unsigned long nextBlinkAtMs_ = 0;
  unsigned long blinkStartMs_ = 0;
  unsigned long blinkDurationMs_ = 0;
  unsigned long blinkSecondStartMs_ = 0;
  unsigned long blinkSecondDurationMs_ = 0;
  unsigned long blinkTotalDurationMs_ = 0;
  bool blinking_ = false;
  bool blinkHasSecondPulse_ = false;
  BlinkType activeBlinkType_ = BlinkType::Normal;
  float blinkBaseUpperLid_ = 0.0f;
  float blinkBaseLowerLid_ = 0.0f;
  float blinkUpperPeak_ = 0.90f;
  float blinkLowerPeak_ = 0.35f;

  // Rich idle modulation
  float idleBreathPhase_ = 0.0f;
  float idleDriftPhase_ = 0.0f;
  float idleAttentionPhase_ = 0.0f;
  unsigned long idleLastUpdateMs_ = 0;
  unsigned long nextIdleMicroAnimAtMs_ = 0;
  unsigned long idleMicroStartMs_ = 0;
  unsigned long idleMicroDurationMs_ = 0;
  IdleMicroAnim idleMicroAnim_ = IdleMicroAnim::None;
  float idleAttentionBias_ = 0.0f;

  unsigned long lastIdleMotionMs_ = 0;
  unsigned long lastSmoothMs_ = 0;
  unsigned long lastEmotionOutputMs_ = 0;
  unsigned long lastEmotionSwitchMs_ = 0;
  unsigned long lastExpressionChangeMs_ = 0;

  float baseOpenness_ = 1.0f;
  FacePresetId activePresetId_ = FacePresetId::Neutral;
  FacePresetId blendFromPresetId_ = FacePresetId::Neutral;
  FacePresetId blendToPresetId_ = FacePresetId::Neutral;
  bool presetBlendActive_ = false;
  unsigned long presetBlendStartMs_ = 0;
  unsigned long presetBlendDurationMs_ = 0;
  unsigned long presetSettleUntilMs_ = 0;
  unsigned long lastPresetSwitchMs_ = 0;

  bool tunerEnabled_ = false;
  FacePresetId tunerPresetId_ = FacePresetId::Neutral;
  ExpressionPreset tunerPreset_{};

  ExpressionType lastEmotionTarget_ = ExpressionType::Neutral;
  FaceRenderState renderState_{};

  PerfStats perf_;
};
