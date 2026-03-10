#pragma once

#include <stddef.h>

#include "../../core/event_bus.h"
#include "../../interfaces/i_event_listener.h"
#include "../../interfaces/i_face_controller.h"
#include "../../models/attention_focus.h"
#include "../../models/mood_profile.h"
#include "../../models/persona_profile.h"
#include "../../models/routine_state.h"

class SimTestInputService : public IEventListener {
public:
  struct ScriptStep {
    unsigned long atMs;
    EventType type;
    int value;
    const char* note;
  };

  SimTestInputService(EventBus& eventBus, IFaceController& faceController);

  void init();
  void update(unsigned long nowMs);
  void onEvent(const Event& event) override;

private:
  void processCommand(char* line, unsigned long nowMs);
  void publish(EventType type, unsigned long nowMs, int value = 0);
  void runScript(const ScriptStep* steps, size_t count, unsigned long nowMs);
  void stopScript();
  void updateScript(unsigned long nowMs);
  void printEventSnapshot(const Event& event);
  void printStatus() const;
  void printHelp() const;

  EventBus& eventBus_;
  IFaceController& faceController_;

  enum class MonitorMode {
    Off = 0,
    Lean,
    Full
  };

  static constexpr size_t kMaxCmdLen = 64;
  char cmdBuffer_[kMaxCmdLen] = {0};
  size_t cmdLen_ = 0;

  MonitorMode monitorMode_ = MonitorMode::Off;
  bool scriptRunning_ = false;
  const ScriptStep* scriptSteps_ = nullptr;
  size_t scriptStepCount_ = 0;
  size_t nextScriptStep_ = 0;
  unsigned long scriptStartMs_ = 0;

  int lastAttention_ = static_cast<int>(AttentionFocus::Idle);
  int lastMoodProfile_ = static_cast<int>(MoodProfile::Calm);
  int lastRoutine_ = static_cast<int>(RoutineState::Idle);
  int lastPersonaTone_ = static_cast<int>(PersonaTone::Warm);
  float lastMood_ = 0.0f;
  float lastEngagement_ = 0.0f;
  float lastAffinity_ = 0.0f;
  float lastSocialResponsiveness_ = 0.0f;
  int lastMemoryTotal_ = 0;
};


