#include "sim_test_input_service.h"

#include "../../models/attention_focus.h"
#include "../../models/event.h"
#include "../../models/intent_types.h"
#include "../../models/mood_profile.h"
#include "../../models/persona_profile.h"
#include "../../models/routine_state.h"
#include <Arduino.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace {

const char* focusName(int value) {
  switch (static_cast<AttentionFocus>(value)) {
    case AttentionFocus::None: return "none";
    case AttentionFocus::Idle: return "idle";
    case AttentionFocus::Touch: return "touch";
    case AttentionFocus::Voice: return "voice";
    case AttentionFocus::Vision: return "vision";
    case AttentionFocus::Power: return "power";
    case AttentionFocus::Internal: return "internal";
    default: return "unknown";
  }
}

const char* moodProfileName(int value) {
  switch (static_cast<MoodProfile>(value)) {
    case MoodProfile::Calm: return "calm";
    case MoodProfile::Animated: return "animated";
    case MoodProfile::Sleepy: return "sleepy";
    case MoodProfile::Curious: return "curious";
    case MoodProfile::Sensitive: return "sensitive";
    case MoodProfile::Social: return "social";
    case MoodProfile::Reserved: return "reserved";
    default: return "unknown";
  }
}

const char* routineName(int value) {
  switch (static_cast<RoutineState>(value)) {
    case RoutineState::None: return "none";
    case RoutineState::Idle: return "idle";
    case RoutineState::Attentive: return "attentive";
    case RoutineState::Calm: return "calm";
    case RoutineState::Curious: return "curious";
    case RoutineState::Sleepy: return "sleepy";
    case RoutineState::Bored: return "bored";
    case RoutineState::Resume: return "resume";
    case RoutineState::Charging: return "charging";
    case RoutineState::Listening: return "listening";
    case RoutineState::Rest: return "rest";
    default: return "unknown";
  }
}

const char* toneName(int value) {
  switch (static_cast<PersonaTone>(value)) {
    case PersonaTone::Warm: return "warm";
    case PersonaTone::Playful: return "playful";
    case PersonaTone::Calm: return "calm";
    default: return "unknown";
  }
}

constexpr SimTestInputService::ScriptStep kTest17FullScript[] = {
    {0, EventType::EVT_TOUCH, 1, "phase1: engage start"},
    {220, EventType::EVT_VOICE_ACTIVITY, 480, nullptr},
    {450, EventType::EVT_INTENT_DETECTED, static_cast<int>(LocalIntent::Hello), nullptr},
    {900, EventType::EVT_TOUCH, 1, nullptr},
    {1300, EventType::EVT_VOICE_ACTIVITY, 520, nullptr},
    {1800, EventType::EVT_INTENT_DETECTED, static_cast<int>(LocalIntent::Status), nullptr},
    {2400, EventType::EVT_TOUCH, 1, "phase2: maintain social"},
    {3000, EventType::EVT_VOICE_ACTIVITY, 470, nullptr},
    {3600, EventType::EVT_TILT, 650, nullptr},
    {7200, EventType::EVT_IDLE, 1, "phase3: user distant"},
    {8400, EventType::EVT_IDLE, 1, nullptr},
    {9600, EventType::EVT_IDLE, 1, nullptr},
    {10800, EventType::EVT_IDLE, 1, nullptr},
    {12000, EventType::EVT_IDLE, 1, nullptr},
    {13200, EventType::EVT_IDLE, 1, nullptr},
    {14400, EventType::EVT_IDLE, 1, nullptr},
    {17200, EventType::EVT_VOICE_ACTIVITY, 450, "phase4: soft return"},
    {17600, EventType::EVT_INTENT_DETECTED, static_cast<int>(LocalIntent::Hello), nullptr},
    {18400, EventType::EVT_TOUCH, 1, nullptr},
    {19600, EventType::EVT_VOICE_ACTIVITY, 440, nullptr},
    {20800, EventType::EVT_IDLE, 1, "phase5: settle"},
    {22000, EventType::EVT_IDLE, 1, nullptr},
    {23200, EventType::EVT_IDLE, 1, nullptr},
};

constexpr SimTestInputService::ScriptStep kTest17HighScript[] = {
    {0, EventType::EVT_TOUCH, 1, "high engagement burst"},
    {250, EventType::EVT_VOICE_ACTIVITY, 500, nullptr},
    {550, EventType::EVT_INTENT_DETECTED, static_cast<int>(LocalIntent::Hello), nullptr},
    {900, EventType::EVT_TOUCH, 1, nullptr},
    {1300, EventType::EVT_VOICE_ACTIVITY, 480, nullptr},
};

constexpr SimTestInputService::ScriptStep kTest17LowScript[] = {
    {0, EventType::EVT_IDLE, 1, "low engagement / distant"},
    {1200, EventType::EVT_IDLE, 1, nullptr},
    {2400, EventType::EVT_IDLE, 1, nullptr},
    {3600, EventType::EVT_IDLE, 1, nullptr},
    {4800, EventType::EVT_IDLE, 1, nullptr},
    {6000, EventType::EVT_IDLE, 1, nullptr},
};

constexpr SimTestInputService::ScriptStep kScenarioMorningScript[] = {
    {0, EventType::EVT_VOICE_ACTIVITY, 420, "morning: soft wake"},
    {400, EventType::EVT_TOUCH, 1, nullptr},
    {900, EventType::EVT_INTENT_DETECTED, static_cast<int>(LocalIntent::Hello), nullptr},
    {1600, EventType::EVT_VOICE_ACTIVITY, 450, nullptr},
    {2600, EventType::EVT_IDLE, 1, "morning: settle"},
};

constexpr SimTestInputService::ScriptStep kScenarioSocialScript[] = {
    {0, EventType::EVT_TOUCH, 1, "social: engage"},
    {300, EventType::EVT_VOICE_ACTIVITY, 520, nullptr},
    {700, EventType::EVT_INTENT_DETECTED, static_cast<int>(LocalIntent::Status), nullptr},
    {1300, EventType::EVT_TOUCH, 1, nullptr},
    {2200, EventType::EVT_VOICE_ACTIVITY, 480, nullptr},
    {3200, EventType::EVT_IDLE, 1, "social: release"},
};

constexpr SimTestInputService::ScriptStep kScenarioStressScript[] = {
    {0, EventType::EVT_SHAKE, 1400, "stress: disturbance"},
    {450, EventType::EVT_TILT, 700, nullptr},
    {900, EventType::EVT_SHAKE, 1500, nullptr},
    {1500, EventType::EVT_FALL, 1, nullptr},
    {2600, EventType::EVT_TOUCH, 1, "stress: recovery"},
    {3400, EventType::EVT_IDLE, 1, nullptr},
};

constexpr SimTestInputService::ScriptStep kScenarioIdleLongScript[] = {
    {0, EventType::EVT_IDLE, 1, "idle_long: no interaction"},
    {3000, EventType::EVT_IDLE, 1, nullptr},
    {6000, EventType::EVT_IDLE, 1, nullptr},
    {9000, EventType::EVT_IDLE, 1, nullptr},
    {12000, EventType::EVT_IDLE, 1, nullptr},
    {15000, EventType::EVT_IDLE, 1, nullptr},
    {18000, EventType::EVT_IDLE, 1, nullptr},
};

} // namespace

SimTestInputService::SimTestInputService(EventBus& eventBus, IFaceController& faceController)
    : eventBus_(eventBus), faceController_(faceController) {}

void SimTestInputService::init() {
  eventBus_.subscribe(EventType::EVT_ENGAGEMENT_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_ATTENTION_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_MOOD_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_MOOD_PROFILE_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_PERSONA_UPDATED, this);
  eventBus_.subscribe(EventType::EVT_AFFINITY_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_SOCIAL_TIMING_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_ROUTINE_STATE_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_BEHAVIOR_ACTION, this);
  eventBus_.subscribe(EventType::EVT_MEMORY_UPDATED, this);

  printHelp();
}

void SimTestInputService::update(unsigned long nowMs) {
  while (Serial.available() > 0) {
    const int raw = Serial.read();
    if (raw < 0) {
      return;
    }

    const char c = static_cast<char>(raw);
    if (c == '\n' || c == '\r') {
      if (cmdLen_ > 0) {
        cmdBuffer_[cmdLen_] = '\0';
        processCommand(cmdBuffer_, nowMs);
        cmdLen_ = 0;
      }
      continue;
    }

    if (cmdLen_ + 1 < kMaxCmdLen) {
      cmdBuffer_[cmdLen_++] = c;
    }
  }

  updateScript(nowMs);
}

void SimTestInputService::onEvent(const Event& event) {
  switch (event.type) {
    case EventType::EVT_ATTENTION_CHANGED:
      lastAttention_ = event.value;
      break;
    case EventType::EVT_MOOD_CHANGED:
      lastMood_ = static_cast<float>(event.value) / 1000.0f;
      break;
    case EventType::EVT_MOOD_PROFILE_CHANGED:
      lastMoodProfile_ = event.value;
      break;
    case EventType::EVT_PERSONA_UPDATED:
      lastPersonaTone_ = event.value;
      break;
    case EventType::EVT_AFFINITY_CHANGED:
      lastAffinity_ = static_cast<float>(event.value) / 1000.0f;
      break;
    case EventType::EVT_SOCIAL_TIMING_CHANGED:
      lastSocialResponsiveness_ = static_cast<float>(event.value) / 1000.0f;
      break;
    case EventType::EVT_ROUTINE_STATE_CHANGED:
      lastRoutine_ = event.value;
      break;
    case EventType::EVT_ENGAGEMENT_CHANGED:
      lastEngagement_ = static_cast<float>(event.value) / 1000.0f;
      break;

    case EventType::EVT_MEMORY_UPDATED:
      lastMemoryTotal_ = event.value;
      break;
    default:
      break;
  }

  if (monitorMode_ == MonitorMode::Off) {
    return;
  }

  printEventSnapshot(event);
}

void SimTestInputService::processCommand(char* line, unsigned long nowMs) {
  if (line == nullptr) {
    return;
  }

  while (*line == ' ' || *line == '\t') {
    line++;
  }

  for (char* p = line; *p != '\0'; ++p) {
    *p = static_cast<char>(tolower(static_cast<unsigned char>(*p)));
  }

  Serial.print("[SIM_TEST] cmd: ");
  Serial.println(line);

  if (strcmp(line, "help") == 0) {
    printHelp();
    return;
  }

  if (strcmp(line, "status") == 0) {
    printStatus();
    return;
  }

  if (strcmp(line, "monitor on") == 0 || strcmp(line, "monitor full") == 0) {
    monitorMode_ = MonitorMode::Full;
    Serial.println("[SIM_TEST] monitor full");
    return;
  }

  if (strcmp(line, "monitor lean") == 0) {
    monitorMode_ = MonitorMode::Lean;
    Serial.println("[SIM_TEST] monitor lean");
    return;
  }

  if (strcmp(line, "monitor off") == 0) {
    monitorMode_ = MonitorMode::Off;
    Serial.println("[SIM_TEST] monitor disabled");
    return;
  }

  if (strcmp(line, "test17 run") == 0) {
    runScript(kTest17FullScript, sizeof(kTest17FullScript) / sizeof(kTest17FullScript[0]), nowMs);
    return;
  }

  if (strcmp(line, "test17 high") == 0) {
    runScript(kTest17HighScript, sizeof(kTest17HighScript) / sizeof(kTest17HighScript[0]), nowMs);
    return;
  }

  if (strcmp(line, "test17 low") == 0) {
    runScript(kTest17LowScript, sizeof(kTest17LowScript) / sizeof(kTest17LowScript[0]), nowMs);
    return;
  }

  if (strcmp(line, "test17 stop") == 0) {
    stopScript();
    return;
  }

  if (strcmp(line, "scenario list") == 0) {
    Serial.println("[SIM_TEST] scenarios: morning | social | stress | idle_long");
    return;
  }

  if (strcmp(line, "scenario morning") == 0) {
    runScript(kScenarioMorningScript, sizeof(kScenarioMorningScript) / sizeof(kScenarioMorningScript[0]), nowMs);
    return;
  }

  if (strcmp(line, "scenario social") == 0) {
    runScript(kScenarioSocialScript, sizeof(kScenarioSocialScript) / sizeof(kScenarioSocialScript[0]), nowMs);
    return;
  }

  if (strcmp(line, "scenario stress") == 0) {
    runScript(kScenarioStressScript, sizeof(kScenarioStressScript) / sizeof(kScenarioStressScript[0]), nowMs);
    return;
  }

  if (strcmp(line, "scenario idle_long") == 0) {
    runScript(kScenarioIdleLongScript, sizeof(kScenarioIdleLongScript) / sizeof(kScenarioIdleLongScript[0]), nowMs);
    return;
  }

  if (strcmp(line, "scenario stop") == 0) {
    stopScript();
    return;
  }

  if (strncmp(line, "face ", 5) == 0) {
    const char* kind = line + 5;
    ExpressionType expr = ExpressionType::Neutral;
    if (strcmp(kind, "neutral") == 0) {
      expr = ExpressionType::Neutral;
    } else if (strcmp(kind, "curious") == 0) {
      expr = ExpressionType::Curiosity;
    } else if (strcmp(kind, "happy") == 0) {
      expr = ExpressionType::FaceRecognized;
    } else if (strcmp(kind, "alert") == 0) {
      expr = ExpressionType::BatteryAlert;
    } else if (strcmp(kind, "angry") == 0) {
      expr = ExpressionType::Angry;
    } else if (strcmp(kind, "sad") == 0) {
      expr = ExpressionType::Sad;
    } else if (strcmp(kind, "surprised") == 0) {
      expr = ExpressionType::Surprised;
    } else if (strcmp(kind, "shy") == 0) {
      expr = ExpressionType::Shy;
    } else if (strcmp(kind, "listening") == 0) {
      expr = ExpressionType::Listening;
    } else if (strcmp(kind, "thinking") == 0) {
      expr = ExpressionType::Thinking;
    } else {
      Serial.println("[SIM_TEST] face unknown");
      return;
    }

    faceController_.requestExpression(expr, EyeAnimPriority::Critical, 3000);
    return;
  }


  if (strcmp(line, "clip cancel") == 0) {
    if (faceController_.cancelClip()) {
      Serial.println("[SIM_TEST] clip canceled");
    } else {
      Serial.println("[SIM_TEST] no active clip");
    }
    return;
  }

  if (strncmp(line, "clip ", 5) == 0) {
    const char* kind = line + 5;
    FaceClipKind clipKind = FaceClipKind::None;
    if (strcmp(kind, "wake_up") == 0) {
      clipKind = FaceClipKind::WakeUp;
    } else if (strcmp(kind, "go_to_sleep") == 0) {
      clipKind = FaceClipKind::GoToSleep;
    } else if (strcmp(kind, "attention_recovery") == 0) {
      clipKind = FaceClipKind::AttentionRecovery;
    } else if (strcmp(kind, "thinking_loop") == 0) {
      clipKind = FaceClipKind::ThinkingLoop;
    } else if (strcmp(kind, "soft_listen") == 0) {
      clipKind = FaceClipKind::SoftListen;
    } else if (strcmp(kind, "shy_retract") == 0) {
      clipKind = FaceClipKind::ShyRetract;
    } else if (strcmp(kind, "happy_ack") == 0) {
      clipKind = FaceClipKind::HappyAck;
    } else {
      Serial.println("[SIM_TEST] clip unknown");
      return;
    }

    if (faceController_.requestClip(clipKind, true)) {
      Serial.println("[SIM_TEST] clip started");
    } else {
      Serial.println("[SIM_TEST] clip blocked by cooldown/ownership");
    }
    return;
  }

  if (strcmp(line, "tuner on") == 0) {
    if (faceController_.tunerSetEnabled(true)) {
      Serial.println("[SIM_TUNER] enabled");
    } else {
      Serial.println("[SIM_TUNER] unsupported");
    }
    return;
  }

  if (strcmp(line, "tuner off") == 0) {
    if (faceController_.tunerSetEnabled(false)) {
      Serial.println("[SIM_TUNER] disabled");
    } else {
      Serial.println("[SIM_TUNER] unsupported");
    }
    return;
  }

  if (strcmp(line, "tuner reset") == 0) {
    if (faceController_.tunerReset()) {
      Serial.println("[SIM_TUNER] reset");
    } else {
      Serial.println("[SIM_TUNER] reset failed");
    }
    return;
  }

  if (strcmp(line, "tuner status") == 0) {
    char status[240] = {0};
    if (faceController_.tunerGetStatus(status, sizeof(status))) {
      Serial.print("[SIM_TUNER] ");
      Serial.println(status);
    } else {
      Serial.println("[SIM_TUNER] status unavailable");
    }
    return;
  }

  if (strncmp(line, "tuner preset ", 13) == 0) {
    const char* preset = line + 13;
    if (faceController_.tunerSetPreset(preset)) {
      Serial.print("[SIM_TUNER] preset=");
      Serial.println(preset);
    } else {
      Serial.println("[SIM_TUNER] invalid preset");
    }
    return;
  }

  if (strncmp(line, "tuner set ", 10) == 0) {
    char key[24] = {0};
    float value = 0.0f;
    const int parsed = sscanf(line + 10, "%23s %f", key, &value);
    if (parsed == 2) {
      if (faceController_.tunerSetParam(key, value)) {
        Serial.print("[SIM_TUNER] ");
        Serial.print(key);
        Serial.print("=");
        Serial.println(value, 4);
      } else {
        Serial.println("[SIM_TUNER] invalid param/value or tuner off");
      }
    } else {
      Serial.println("[SIM_TUNER] usage: tuner set <param> <value>");
    }
    return;
  }

  if (strcmp(line, "touch") == 0) {
    publish(EventType::EVT_TOUCH, nowMs, 1);
    return;
  }

  if (strcmp(line, "shake") == 0) {
    publish(EventType::EVT_SHAKE, nowMs, 1200);
    return;
  }

  if (strcmp(line, "tilt") == 0) {
    publish(EventType::EVT_TILT, nowMs, 600);
    return;
  }

  if (strcmp(line, "fall") == 0) {
    publish(EventType::EVT_FALL, nowMs, 1);
    return;
  }

  if (strcmp(line, "idle") == 0) {
    publish(EventType::EVT_IDLE, nowMs, 1);
    return;
  }

  if (strcmp(line, "voice") == 0) {
    publish(EventType::EVT_VOICE_ACTIVITY, nowMs, 450);
    return;
  }

  if (strncmp(line, "intent ", 7) == 0) {
    const char* kind = line + 7;
    if (strcmp(kind, "hello") == 0) {
      publish(EventType::EVT_INTENT_DETECTED, nowMs, static_cast<int>(LocalIntent::Hello));
      return;
    }
    if (strcmp(kind, "status") == 0) {
      publish(EventType::EVT_INTENT_DETECTED, nowMs, static_cast<int>(LocalIntent::Status));
      return;
    }
    if (strcmp(kind, "sleep") == 0) {
      publish(EventType::EVT_INTENT_DETECTED, nowMs, static_cast<int>(LocalIntent::Sleep));
      return;
    }
    if (strcmp(kind, "wake") == 0) {
      publish(EventType::EVT_INTENT_DETECTED, nowMs, static_cast<int>(LocalIntent::Wake));
      return;
    }
    if (strcmp(kind, "photo") == 0) {
      publish(EventType::EVT_INTENT_DETECTED, nowMs, static_cast<int>(LocalIntent::Photo));
      return;
    }
  }

  if (strncmp(line, "ota check", 9) == 0) {
    int version = 2;
    const char* p = line + 9;
    while (*p == ' ') {
      p++;
    }
    if (*p != '\0') {
      const int parsed = atoi(p);
      if (parsed > 1) {
        version = parsed;
      }
    }

    publish(EventType::EVT_OTA_CHECK_REQUEST, nowMs, version);
    return;
  }

  if (strcmp(line, "ota apply") == 0) {
    publish(EventType::EVT_OTA_APPLY_REQUEST, nowMs, 1);
    return;
  }

  if (strcmp(line, "safe on") == 0) {
    publish(EventType::EVT_SAFE_MODE_CHANGED, nowMs, 1);
    return;
  }

  if (strcmp(line, "safe off") == 0) {
    publish(EventType::EVT_SAFE_MODE_CHANGED, nowMs, 0);
    return;
  }

  Serial.println("[SIM_TEST] unknown command");
}

void SimTestInputService::publish(EventType type, unsigned long nowMs, int value) {
  Event event;
  event.type = type;
  event.source = EventSource::System;
  event.value = value;
  event.timestamp = nowMs;
  eventBus_.publish(event);
}

void SimTestInputService::runScript(const ScriptStep* steps, size_t count, unsigned long nowMs) {
  if (steps == nullptr || count == 0) {
    return;
  }

  scriptSteps_ = steps;
  scriptStepCount_ = count;
  nextScriptStep_ = 0;
  scriptStartMs_ = nowMs;
  scriptRunning_ = true;

  Serial.print("[SIM_TEST] script started, steps=");
  Serial.println(static_cast<int>(count));
}

void SimTestInputService::stopScript() {
  if (!scriptRunning_) {
    Serial.println("[SIM_TEST] script not running");
    return;
  }

  scriptRunning_ = false;
  scriptSteps_ = nullptr;
  scriptStepCount_ = 0;
  nextScriptStep_ = 0;
  Serial.println("[SIM_TEST] script stopped");
}

void SimTestInputService::updateScript(unsigned long nowMs) {
  if (!scriptRunning_ || scriptSteps_ == nullptr || nextScriptStep_ >= scriptStepCount_) {
    return;
  }

  const unsigned long elapsed = nowMs - scriptStartMs_;
  while (nextScriptStep_ < scriptStepCount_ && elapsed >= scriptSteps_[nextScriptStep_].atMs) {
    const ScriptStep& step = scriptSteps_[nextScriptStep_];
    if (step.note != nullptr) {
      Serial.print("[SIM_TEST] ");
      Serial.println(step.note);
    }

    if (step.type != EventType::None) {
      publish(step.type, nowMs, step.value);
    }

    nextScriptStep_++;
  }

  if (nextScriptStep_ >= scriptStepCount_) {
    scriptRunning_ = false;
    Serial.println("[SIM_TEST] script completed");
  }
}

void SimTestInputService::printEventSnapshot(const Event& event) {
  if (monitorMode_ == MonitorMode::Lean) {
    const bool isLeanEvent =
        event.type == EventType::EVT_ATTENTION_CHANGED ||
        event.type == EventType::EVT_MOOD_PROFILE_CHANGED ||
        event.type == EventType::EVT_ROUTINE_STATE_CHANGED;
    if (!isLeanEvent) {
      return;
    }
  }

  switch (event.type) {
    case EventType::EVT_ENGAGEMENT_CHANGED:
      Serial.print("[SIM_MON] engagement=");
      Serial.println(static_cast<float>(event.value) / 1000.0f, 3);
      break;

    case EventType::EVT_ATTENTION_CHANGED:
      Serial.print("[SIM_MON] attention=");
      Serial.println(focusName(event.value));
      break;

    case EventType::EVT_MOOD_CHANGED:
      Serial.print("[SIM_MON] mood=");
      Serial.println(static_cast<float>(event.value) / 1000.0f, 3);
      break;

    case EventType::EVT_MOOD_PROFILE_CHANGED:
      Serial.print("[SIM_MON] mood_profile=");
      Serial.println(moodProfileName(event.value));
      break;

    case EventType::EVT_PERSONA_UPDATED:
      Serial.print("[SIM_MON] persona_tone=");
      Serial.println(toneName(event.value));
      break;

    case EventType::EVT_AFFINITY_CHANGED:
      Serial.print("[SIM_MON] affinity=");
      Serial.println(static_cast<float>(event.value) / 1000.0f, 3);
      break;

    case EventType::EVT_SOCIAL_TIMING_CHANGED:
      Serial.print("[SIM_MON] social_responsiveness=");
      Serial.println(static_cast<float>(event.value) / 1000.0f, 3);
      break;

    case EventType::EVT_ROUTINE_STATE_CHANGED:
      Serial.print("[SIM_MON] routine=");
      Serial.println(routineName(event.value));
      break;

    case EventType::EVT_BEHAVIOR_ACTION:
      Serial.print("[SIM_MON] behavior_action=");
      Serial.println(event.value);
      break;


    case EventType::EVT_MEMORY_UPDATED:
      Serial.print("[SIM_MON] memory_total=");
      Serial.println(event.value);
      break;

    default:
      break;
  }
}

void SimTestInputService::printStatus() const {
  Serial.println("[SIM_STATUS]");
  Serial.print("  routine=");
  Serial.println(routineName(lastRoutine_));
  Serial.print("  attention=");
  Serial.println(focusName(lastAttention_));
  Serial.print("  mood_profile=");
  Serial.println(moodProfileName(lastMoodProfile_));
  Serial.print("  mood=");
  Serial.println(lastMood_, 3);
  Serial.print("  engagement=");
  Serial.println(lastEngagement_, 3);
  Serial.print("  affinity=");
  Serial.println(lastAffinity_, 3);
  Serial.print("  social_responsiveness=");
  Serial.println(lastSocialResponsiveness_, 3);
  Serial.print("  persona_tone=");
  Serial.println(toneName(lastPersonaTone_));
  Serial.print("  memory_total=");
  Serial.println(lastMemoryTotal_);
  Serial.print("  script_running=");
  Serial.println(scriptRunning_ ? "yes" : "no");
  Serial.print("  monitor_mode=");
  switch (monitorMode_) {
    case MonitorMode::Off: Serial.println("off"); break;
    case MonitorMode::Lean: Serial.println("lean"); break;
    case MonitorMode::Full: Serial.println("full"); break;
  }
}
void SimTestInputService::printHelp() const {
  Serial.println("[SIM_TEST] commands:");
  Serial.println("  test17 run | test17 high | test17 low | test17 stop");
  Serial.println("  scenario list | scenario morning|social|stress|idle_long|stop");
  Serial.println("  monitor on|off|lean|full");
  Serial.println("  status");
  Serial.println("  face neutral|curious|happy|alert|angry|sad|surprised|shy|listening|thinking");
  Serial.println("  clip wake_up|go_to_sleep|attention_recovery|thinking_loop|soft_listen|shy_retract|happy_ack|cancel");
  Serial.println("  tuner on|off|status|reset");
  Serial.println("  tuner preset neutral_premium|curious_soft|sleepy_sink|warm_happy|focused_listen|shy_peek|surprised_open|low_energy_flat|attention_lock|thinking_side");
  Serial.println("  tuner set <param> <value>  (width height round tilt space upper lower open)");
  Serial.println("  touch | shake | tilt | fall | idle | voice");
  Serial.println("  intent hello|status|sleep|wake|photo");
  Serial.println("  safe on|off");
  Serial.println("  ota check [version]");
  Serial.println("  ota apply");
  Serial.println("  presets legacy aliases still accepted: neutral|curious|sleepy|happy_soft|focused|shy|surprised|low_energy|listening|thinking");
  Serial.println("  help");
}



















