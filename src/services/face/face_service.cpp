#include "face_service.h"
#include "../../config/hardware_config.h"
#include <Arduino.h>

FaceService::FaceService(DisplayHAL& displayHal)
  : displayHal_(displayHal),
    renderer_(displayHal) {
}

void FaceService::init() {
  randomSeed(millis());

  leftEye_.centerX = HardwareConfig::Face::LEFT_EYE_X;
  leftEye_.centerY = HardwareConfig::Face::EYE_Y;
  leftEye_.eyeRadius = HardwareConfig::Face::EYE_RADIUS;
  leftEye_.pupilRadius = HardwareConfig::Face::PUPIL_RADIUS;
  leftEye_.openness = 1.0f;

  rightEye_.centerX = HardwareConfig::Face::RIGHT_EYE_X;
  rightEye_.centerY = HardwareConfig::Face::EYE_Y;
  rightEye_.eyeRadius = HardwareConfig::Face::EYE_RADIUS;
  rightEye_.pupilRadius = HardwareConfig::Face::PUPIL_RADIUS;
  rightEye_.openness = 1.0f;

  displayHal_.init();

  scheduleNextBlink(millis());
  lastLookChangeMs_ = millis();

  renderer_.render(leftEye_, rightEye_);
}

void FaceService::update(unsigned long nowMs) {
  updateBlink(nowMs);
  updateIdleLook(nowMs);
  renderer_.render(leftEye_, rightEye_);
}

void FaceService::scheduleNextBlink(unsigned long nowMs) {
  const unsigned long interval = random(
    HardwareConfig::Face::BLINK_MIN_INTERVAL_MS,
    HardwareConfig::Face::BLINK_MAX_INTERVAL_MS
  );
  nextBlinkAtMs_ = nowMs + interval;
}

void FaceService::updateBlink(unsigned long nowMs) {
  if (!blinking_ && nowMs >= nextBlinkAtMs_) {
    blinking_ = true;
    blinkStartMs_ = nowMs;
  }

  if (!blinking_) {
    leftEye_.openness = 1.0f;
    rightEye_.openness = 1.0f;
    return;
  }

  const unsigned long elapsed = nowMs - blinkStartMs_;

  if (elapsed < 50) {
    leftEye_.openness = 1.0f;
    rightEye_.openness = 1.0f;
  } else if (elapsed < 100) {
    leftEye_.openness = 0.6f;
    rightEye_.openness = 0.6f;
  } else if (elapsed < 150) {
    leftEye_.openness = 0.15f;
    rightEye_.openness = 0.15f;
  } else if (elapsed < 200) {
    leftEye_.openness = 0.6f;
    rightEye_.openness = 0.6f;
  } else if (elapsed < 240) {
    leftEye_.openness = 1.0f;
    rightEye_.openness = 1.0f;
  } else {
    blinking_ = false;
    leftEye_.openness = 1.0f;
    rightEye_.openness = 1.0f;
    scheduleNextBlink(nowMs);
  }
}

void FaceService::updateIdleLook(unsigned long nowMs) {
  if (nowMs - lastLookChangeMs_ < HardwareConfig::Face::LOOK_INTERVAL_MS) {
    return;
  }

  lastLookChangeMs_ = nowMs;

  const int offsetX = random(
    -HardwareConfig::Face::MAX_PUPIL_OFFSET_X,
     HardwareConfig::Face::MAX_PUPIL_OFFSET_X + 1
  );

  const int offsetY = random(
    -HardwareConfig::Face::MAX_PUPIL_OFFSET_Y,
     HardwareConfig::Face::MAX_PUPIL_OFFSET_Y + 1
  );

  leftEye_.pupilOffsetX = offsetX;
  leftEye_.pupilOffsetY = offsetY;

  rightEye_.pupilOffsetX = offsetX;
  rightEye_.pupilOffsetY = offsetY;
}