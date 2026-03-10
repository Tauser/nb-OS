#include "mood_service.h"

#include "../../config/hardware_config.h"
#include "../../models/event.h"
#include "../../utils/math_utils.h"
#include <Arduino.h>
#include <math.h>

namespace {
float approach(float current, float target, float ratePerS, float dtS) {
  const float alpha = MathUtils::clamp(ratePerS * dtS, 0.0f, 1.0f);
  return current + ((target - current) * alpha);
}
}

MoodService::MoodService(EventBus& eventBus, const IEmotionProvider& emotionProvider)
    : eventBus_(eventBus), emotionProvider_(emotionProvider) {}

void MoodService::init() {
  eventBus_.subscribe(EventType::EVT_TOUCH, this);
  eventBus_.subscribe(EventType::EVT_VOICE_START, this);
  eventBus_.subscribe(EventType::EVT_VOICE_ACTIVITY, this);
  eventBus_.subscribe(EventType::EVT_INTENT_DETECTED, this);
  eventBus_.subscribe(EventType::EVT_FALL, this);
  eventBus_.subscribe(EventType::EVT_IDLE, this);
  eventBus_.subscribe(EventType::EVT_POWER_MODE_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_ROUTINE_STATE_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_MEMORY_UPDATED, this);

  const unsigned long nowMs = millis();
  lastUpdateMs_ = nowMs;
  lastInteractionMs_ = nowMs;
  state_.profile = classifyProfile();
  publishMood(nowMs);
  publishMoodProfile(nowMs);
}

void MoodService::update(unsigned long nowMs) {
  if (nowMs - lastUpdateMs_ < HardwareConfig::Companion::MOOD_UPDATE_INTERVAL_MS) {
    return;
  }

  const float dtS = static_cast<float>(nowMs - lastUpdateMs_) / 1000.0f;
  lastUpdateMs_ = nowMs;

  const EmotionState& emo = emotionProvider_.getEmotionState();
  const unsigned long silenceMs = nowMs - lastInteractionMs_;

  state_.valence = approach(state_.valence, emo.valence, 0.12f, dtS);
  state_.valence = approach(state_.valence, 0.0f, HardwareConfig::Companion::MOOD_DECAY_PER_S, dtS);

  if (silenceMs >= HardwareConfig::Companion::MOOD_LONG_IDLE_MS) {
    state_.valence -= HardwareConfig::Companion::MOOD_IDLE_NEG_PER_S * dtS;
    state_.reserve += 0.05f * dtS;
  }

  const float emotionStability = 1.0f - fabsf(emo.arousal - 0.5f);
  state_.stability = approach(state_.stability, emotionStability, 0.18f, dtS);

  const float socialTarget = MathUtils::clamp((emo.bond * 0.45f) + (emo.attention * 0.35f) + (memorySignal_ * 0.20f), 0.0f, 1.0f);
  state_.socialDrive = approach(state_.socialDrive, socialTarget, 0.10f, dtS);

  const float reserveTarget = MathUtils::clamp(((1.0f - emo.energy) * 0.55f) + ((silenceMs >= HardwareConfig::Companion::MOOD_LONG_IDLE_MS) ? 0.30f : 0.0f), 0.0f, 1.0f);
  state_.reserve = approach(state_.reserve, reserveTarget, 0.08f, dtS);

  state_.energyTrend = approach(state_.energyTrend, emo.energy, 0.14f, dtS);
  state_.curiosityTrend = approach(state_.curiosityTrend, emo.curiosity, 0.16f, dtS);

  if (routineState_ == RoutineState::Sleepy || powerMode_ == PowerMode::Sleep) {
    state_.energyTrend = approach(state_.energyTrend, 0.20f, 0.20f, dtS);
    state_.reserve = approach(state_.reserve, 0.75f, 0.18f, dtS);
  }

  if (powerMode_ == PowerMode::Charging) {
    state_.socialDrive = approach(state_.socialDrive, 0.55f, 0.08f, dtS);
    state_.reserve = approach(state_.reserve, 0.45f, 0.08f, dtS);
  }

  state_.clamp();

  const MoodProfile newProfile = classifyProfile();
  const bool profileChanged = (newProfile != state_.profile);
  state_.profile = newProfile;

  const float dValence = fabsf(state_.valence - lastPublished_.valence);
  const float dStability = fabsf(state_.stability - lastPublished_.stability);
  const float dSocial = fabsf(state_.socialDrive - lastPublished_.socialDrive);

  if (dValence >= HardwareConfig::Companion::MOOD_PUBLISH_THRESHOLD ||
      dStability >= (HardwareConfig::Companion::MOOD_PUBLISH_THRESHOLD * 0.75f) ||
      dSocial >= (HardwareConfig::Companion::MOOD_PUBLISH_THRESHOLD * 0.75f)) {
    publishMood(nowMs);
  }

  if (profileChanged || state_.profile != lastPublishedProfile_) {
    publishMoodProfile(nowMs);
  }
}

void MoodService::onEvent(const Event& event) {
  const unsigned long nowMs = event.timestamp > 0 ? event.timestamp : millis();

  switch (event.type) {
    case EventType::EVT_TOUCH:
      markInteraction(nowMs);
      state_.valence += 0.05f;
      state_.socialDrive += 0.05f;
      break;

    case EventType::EVT_VOICE_START:
    case EventType::EVT_VOICE_ACTIVITY:
    case EventType::EVT_INTENT_DETECTED:
      markInteraction(nowMs);
      state_.valence += 0.03f;
      state_.socialDrive += 0.04f;
      break;

    case EventType::EVT_FALL:
      state_.valence -= 0.10f;
      state_.stability -= 0.08f;
      state_.reserve += 0.10f;
      break;

    case EventType::EVT_IDLE:
      state_.reserve += 0.01f;
      break;

    case EventType::EVT_POWER_MODE_CHANGED:
      powerMode_ = static_cast<PowerMode>(event.value);
      break;

    case EventType::EVT_ROUTINE_STATE_CHANGED:
      routineState_ = static_cast<RoutineState>(event.value);
      break;

    case EventType::EVT_MEMORY_UPDATED: {
      const float norm = MathUtils::clamp(static_cast<float>(event.value) / 1000.0f, 0.0f, 1.0f);
      memorySignal_ = approach(memorySignal_, norm, 0.25f, 1.0f);
      break;
    }

    default:
      break;
  }

  state_.clamp();
}

const MoodState& MoodService::getState() const {
  return state_;
}

void MoodService::publishMood(unsigned long nowMs) {
  Event event;
  event.type = EventType::EVT_MOOD_CHANGED;
  event.source = EventSource::MoodService;
  event.timestamp = nowMs;

  MoodChangedPayload payload;
  payload.valence = state_.valence;
  payload.profile = static_cast<int>(state_.profile);
  payload.stability = state_.stability;
  payload.socialDrive = state_.socialDrive;
  payload.reserve = state_.reserve;
  event.setMoodChanged(payload);

  eventBus_.publish(event);

  lastPublished_ = state_;
}

void MoodService::publishMoodProfile(unsigned long nowMs) {
  Event event;
  event.type = EventType::EVT_MOOD_PROFILE_CHANGED;
  event.source = EventSource::MoodService;
  event.value = static_cast<int>(state_.profile);
  event.timestamp = nowMs;
  eventBus_.publish(event);

  lastPublishedProfile_ = state_.profile;
}

void MoodService::markInteraction(unsigned long nowMs) {
  lastInteractionMs_ = nowMs;
}

MoodProfile MoodService::classifyProfile() const {
  if (state_.energyTrend < 0.32f || routineState_ == RoutineState::Sleepy || powerMode_ == PowerMode::Sleep) {
    return MoodProfile::Sleepy;
  }

  if (state_.socialDrive > 0.65f && state_.valence > -0.10f) {
    return MoodProfile::Social;
  }

  if (state_.reserve > 0.68f && state_.socialDrive < 0.50f) {
    return MoodProfile::Reserved;
  }

  if (state_.curiosityTrend > 0.68f) {
    return MoodProfile::Curious;
  }

  if (state_.stability < 0.35f || state_.valence < -0.38f) {
    return MoodProfile::Sensitive;
  }

  if (state_.energyTrend > 0.72f && state_.valence > 0.15f) {
    return MoodProfile::Animated;
  }

  return MoodProfile::Calm;
}

