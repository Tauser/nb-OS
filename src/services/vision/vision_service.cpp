#include "vision_service.h"

#include "../../config/feature_flags.h"
#include "../../config/hardware_config.h"
#include "../../models/cloud_types.h"
#include "../../models/event.h"

#include <Arduino.h>

#ifndef NCOS_SIM_MODE
#define NCOS_SIM_MODE 0
#endif

VisionService::VisionService(ICameraPort& cameraPort, EventBus& eventBus)
    : cameraPort_(cameraPort), eventBus_(eventBus) {}

void VisionService::init() {
  cameraPort_.init();
  metricsWindowStartMs_ = millis();
}

void VisionService::update() {
  VisionSnapshot snapshot;
  if (!cameraPort_.sampleFrame(snapshot) || !snapshot.valid) {
    metricsCaptureFail_++;
    const unsigned long nowMs = millis();
    if (nowMs - metricsWindowStartMs_ >= HardwareConfig::Vision::METRICS_WINDOW_MS) {
      emitMetrics(nowMs);
    }
    return;
  }

  updateMetrics(snapshot);

  const unsigned long nowMs = snapshot.timestampMs;
  publishVisionEvent(EventType::EVT_VISION_SNAPSHOT, static_cast<int>(snapshot.avgLuma), nowMs);

  if (snapshot.avgLuma <= HardwareConfig::Vision::DARK_LUMA_THRESHOLD &&
      nowMs - lastVisionEventMs_ >= HardwareConfig::Vision::EVENT_COOLDOWN_MS) {
    publishVisionEvent(EventType::EVT_VISION_DARK, static_cast<int>(snapshot.avgLuma), nowMs);
    requestCloud(CloudRequestType::VisionDark, nowMs);
    lastVisionEventMs_ = nowMs;
  }

  if (snapshot.motionScore >= HardwareConfig::Vision::MOTION_SCORE_THRESHOLD &&
      nowMs - lastVisionEventMs_ >= HardwareConfig::Vision::EVENT_COOLDOWN_MS) {
    publishVisionEvent(EventType::EVT_VISION_MOTION, static_cast<int>(snapshot.motionScore), nowMs);
    requestCloud(CloudRequestType::VisionMotion, nowMs);
    lastVisionEventMs_ = nowMs;
  }

  if (nowMs - metricsWindowStartMs_ >= HardwareConfig::Vision::METRICS_WINDOW_MS) {
    emitMetrics(nowMs);
  }
}

void VisionService::publishVisionEvent(EventType type, int value, unsigned long nowMs) {
  Event event;
  event.type = type;
  event.source = EventSource::VisionService;
  event.value = value;
  event.timestamp = nowMs;
  eventBus_.publish(event);
}

void VisionService::requestCloud(CloudRequestType requestType, unsigned long nowMs) {
  if (!FeatureFlags::CLOUD_ENABLED) {
    return;
  }

  if (nowMs - lastCloudRequestMs_ < HardwareConfig::Cloud::REQUEST_COOLDOWN_MS) {
    return;
  }

  Event event;
  event.type = EventType::EVT_CLOUD_REQUEST;
  event.source = EventSource::VisionService;
  event.value = static_cast<int>(requestType);
  event.timestamp = nowMs;
  eventBus_.publish(event);

  lastCloudRequestMs_ = nowMs;
}

void VisionService::updateMetrics(const VisionSnapshot& snapshot) {
  metricsFrames_++;
  if (snapshot.realBackend) {
    metricsRealFrames_++;
  } else {
    metricsSyntheticFrames_++;
  }

  metricsCaptureTotalUs_ += snapshot.captureUs;
  if (snapshot.captureUs > metricsCaptureMaxUs_) {
    metricsCaptureMaxUs_ = snapshot.captureUs;
  }
}

void VisionService::emitMetrics(unsigned long nowMs) {
  const uint32_t elapsedMs = (nowMs > metricsWindowStartMs_) ? (nowMs - metricsWindowStartMs_) : 1U;
  const uint32_t fps100 = (metricsFrames_ * 100000U) / elapsedMs;
  const uint32_t avgCaptureUs = (metricsFrames_ > 0U) ? (metricsCaptureTotalUs_ / metricsFrames_) : 0U;

  Serial.printf("[VISION_METRICS] frames=%lu fps=%lu.%02lu real=%lu synthetic=%lu fail=%lu avg_us=%lu max_us=%lu\n",
                static_cast<unsigned long>(metricsFrames_),
                static_cast<unsigned long>(fps100 / 100U),
                static_cast<unsigned long>(fps100 % 100U),
                static_cast<unsigned long>(metricsRealFrames_),
                static_cast<unsigned long>(metricsSyntheticFrames_),
                static_cast<unsigned long>(metricsCaptureFail_),
                static_cast<unsigned long>(avgCaptureUs),
                static_cast<unsigned long>(metricsCaptureMaxUs_));

  metricsWindowStartMs_ = nowMs;
  metricsFrames_ = 0;
  metricsRealFrames_ = 0;
  metricsSyntheticFrames_ = 0;
  metricsCaptureFail_ = 0;
  metricsCaptureTotalUs_ = 0;
  metricsCaptureMaxUs_ = 0;
}
