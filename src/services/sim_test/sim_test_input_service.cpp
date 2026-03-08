#include "sim_test_input_service.h"

#include "../../models/event.h"
#include "../../models/intent_types.h"
#include <Arduino.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

SimTestInputService::SimTestInputService(EventBus& eventBus, IFaceController& faceController)
    : eventBus_(eventBus), faceController_(faceController) {}

void SimTestInputService::init() {
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
    } else {
      Serial.println("[SIM_TEST] face unknown");
      return;
    }

    faceController_.requestExpression(expr, EyeAnimPriority::Critical, 3000);
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

void SimTestInputService::printHelp() const {
  Serial.println("[SIM_TEST] commands:");
  Serial.println("  face neutral|curious|happy|alert|angry|sad");
  Serial.println("  touch | shake | tilt | fall | idle | voice");
  Serial.println("  intent hello|status|sleep|wake|photo");
  Serial.println("  safe on|off");
  Serial.println("  ota check [version]");
  Serial.println("  ota apply");
  Serial.println("  help");
}
