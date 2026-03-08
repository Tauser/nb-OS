#include "memory_service.h"

#include "../../config/hardware_config.h"
#include "../../models/event.h"
#include "../../models/routine_state.h"
#include "../../utils/math_utils.h"
#include <Arduino.h>

namespace {
constexpr uint32_t kBehaviorActionSocialCheckIn = 14;

uint32_t saturatingAdd(uint32_t base, uint32_t delta) {
  const uint32_t maxVal = 0xFFFFFFFFu;
  if (delta > (maxVal - base)) {
    return maxVal;
  }
  return base + delta;
}
}

MemoryService::MemoryService(EventBus& eventBus, StorageManager& storageManager)
    : eventBus_(eventBus), storageManager_(storageManager) {}

void MemoryService::init() {
  eventBus_.subscribe(EventType::EVT_TOUCH, this);
  eventBus_.subscribe(EventType::EVT_VOICE_ACTIVITY, this);
  eventBus_.subscribe(EventType::EVT_INTENT_DETECTED, this);
  eventBus_.subscribe(EventType::EVT_ATTENTION_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_IDLE, this);
  eventBus_.subscribe(EventType::EVT_ROUTINE_STATE_CHANGED, this);
  eventBus_.subscribe(EventType::EVT_BEHAVIOR_ACTION, this);

  storageManager_.loadLongTermMemory(profile_.longTerm);
  profile_.longTerm.sanitize();
  profile_.longTerm.totalSessions = saturatingAdd(profile_.longTerm.totalSessions, 1U);
  dirtyLongTerm_ = true;

  const unsigned long nowMs = millis();
  profile_.session.sessionStartMs = static_cast<uint32_t>(nowMs);
  profile_.session.lastInteractionMs = static_cast<uint32_t>(nowMs);
  lastUpdateMs_ = nowMs;
  lastPersistMs_ = nowMs;
  lastSummaryMs_ = nowMs;
  lastMergedSessionDurationMs_ = 0;

  rebuildPreference();
  publishPreference(nowMs);
  publishMemoryStatus(nowMs);
}

void MemoryService::update(unsigned long nowMs) {
  if (nowMs - lastUpdateMs_ < HardwareConfig::Memory::UPDATE_INTERVAL_MS) {
    return;
  }

  lastUpdateMs_ = nowMs;
  mergeSessionIntoLongTerm(nowMs);
  rebuildPreference();

  if (profile_.longTerm.preferredFocus != lastPublishedFocus_ ||
      profile_.longTerm.prefersCalmStyle != lastPublishedCalmStyle_) {
    publishPreference(nowMs);
  }

  if (nowMs - lastSummaryMs_ >= HardwareConfig::Memory::SUMMARY_INTERVAL_MS) {
    publishMemoryStatus(nowMs);
    lastSummaryMs_ = nowMs;
  }

  if (dirtyLongTerm_ && nowMs - lastPersistMs_ >= HardwareConfig::Memory::PERSIST_INTERVAL_MS) {
    if (storageManager_.saveLongTermMemory(profile_.longTerm)) {
      dirtyLongTerm_ = false;
      lastPersistMs_ = nowMs;
    }
  }
}

void MemoryService::onEvent(const Event& event) {
  profile_.shortTerm.push(event.type);
  const unsigned long nowMs = (event.timestamp > 0) ? event.timestamp : millis();

  switch (event.type) {
    case EventType::EVT_TOUCH:
      profile_.session.totalInteractions++;
      profile_.session.touchInteractions++;
      profile_.session.markInteraction(nowMs);

      profile_.longTerm.totalInteractions = saturatingAdd(profile_.longTerm.totalInteractions, 1U);
      profile_.longTerm.touchInteractions = saturatingAdd(profile_.longTerm.touchInteractions, 1U);
      dirtyLongTerm_ = true;
      break;

    case EventType::EVT_VOICE_ACTIVITY:
      profile_.session.totalInteractions++;
      profile_.session.voiceInteractions++;
      profile_.session.markInteraction(nowMs);

      profile_.longTerm.totalInteractions = saturatingAdd(profile_.longTerm.totalInteractions, 1U);
      profile_.longTerm.voiceInteractions = saturatingAdd(profile_.longTerm.voiceInteractions, 1U);
      dirtyLongTerm_ = true;
      break;

    case EventType::EVT_INTENT_DETECTED: {
      profile_.session.totalInteractions++;
      profile_.session.intentInteractions++;
      profile_.session.markInteraction(nowMs);

      profile_.longTerm.totalInteractions = saturatingAdd(profile_.longTerm.totalInteractions, 1U);
      profile_.longTerm.intentInteractions = saturatingAdd(profile_.longTerm.intentInteractions, 1U);

      const int intentVal = event.value;
      if (intentVal >= 0 && intentVal < 16) {
        profile_.longTerm.recurringIntentMask |= static_cast<uint16_t>(1U << intentVal);
        profile_.session.dominantIntent = static_cast<uint8_t>(intentVal);
      }
      dirtyLongTerm_ = true;
      break;
    }

    case EventType::EVT_ATTENTION_CHANGED: {
      const AttentionFocus focus = static_cast<AttentionFocus>(event.value);
      if (focus == AttentionFocus::Vision) {
        profile_.session.visionInteractions++;
        profile_.session.markInteraction(nowMs);

        profile_.longTerm.visionInteractions = saturatingAdd(profile_.longTerm.visionInteractions, 1U);
        dirtyLongTerm_ = true;
      }
      break;
    }

    case EventType::EVT_IDLE:
      profile_.session.idleTicks++;
      break;

    case EventType::EVT_ROUTINE_STATE_CHANGED:
      if (static_cast<RoutineState>(event.value) == RoutineState::Bored) {
        profile_.session.boredEpisodes++;
        profile_.longTerm.boredomEpisodes = saturatingAdd(profile_.longTerm.boredomEpisodes, 1U);
        dirtyLongTerm_ = true;
      }
      break;

    case EventType::EVT_BEHAVIOR_ACTION:
      if (static_cast<uint32_t>(event.value) == kBehaviorActionSocialCheckIn) {
        profile_.session.totalInteractions++;
        profile_.session.markInteraction(nowMs);
        profile_.longTerm.totalInteractions = saturatingAdd(profile_.longTerm.totalInteractions, 1U);
        dirtyLongTerm_ = true;
      }
      break;

    default:
      break;
  }
}

const MemoryProfile& MemoryService::getProfile() const {
  return profile_;
}

void MemoryService::rebuildPreference() {
  const uint32_t touchScore = profile_.session.touchInteractions + (profile_.longTerm.touchInteractions / 5U);
  const uint32_t voiceScore = profile_.session.voiceInteractions + profile_.session.intentInteractions +
                              (profile_.longTerm.voiceInteractions / 5U) + (profile_.longTerm.intentInteractions / 5U);
  const uint32_t visionScore = profile_.session.visionInteractions + (profile_.longTerm.visionInteractions / 5U);

  AttentionFocus focus = AttentionFocus::Touch;
  uint32_t best = touchScore;

  if (voiceScore >= best) {
    best = voiceScore;
    focus = AttentionFocus::Voice;
  }

  if (visionScore > best) {
    best = visionScore;
    focus = AttentionFocus::Vision;
  }

  if (touchScore == 0U && voiceScore == 0U && visionScore == 0U) {
    focus = AttentionFocus::Idle;
  }

  const uint32_t totalScore = touchScore + voiceScore + visionScore;
  float confidence = 0.0f;
  if (totalScore > 0U) {
    confidence = static_cast<float>(best) / static_cast<float>(totalScore);
  }

  const bool calmByVoice = (voiceScore > (touchScore + visionScore));

  profile_.session.dominantFocus = focus;
  profile_.session.prefersCalmStyle = calmByVoice;

  profile_.longTerm.preferredFocus = focus;
  if (calmByVoice) {
    if (profile_.longTerm.calmStyleScore < 100) {
      profile_.longTerm.calmStyleScore = static_cast<uint8_t>(profile_.longTerm.calmStyleScore + 1);
    }
  } else if (profile_.longTerm.calmStyleScore > 0) {
    profile_.longTerm.calmStyleScore = static_cast<uint8_t>(profile_.longTerm.calmStyleScore - 1);
  }
  profile_.longTerm.prefersCalmStyle = (profile_.longTerm.calmStyleScore >= 50);

  profile_.preference.preferredFocus = focus;
  profile_.preference.prefersCalmStyle = profile_.longTerm.prefersCalmStyle;
  profile_.preference.touchInteractions = profile_.longTerm.touchInteractions;
  profile_.preference.voiceInteractions = profile_.longTerm.voiceInteractions + profile_.longTerm.intentInteractions;
  profile_.preference.visionInteractions = profile_.longTerm.visionInteractions;
  profile_.preference.intentInteractions = profile_.longTerm.intentInteractions;
  profile_.preference.focusConfidence = MathUtils::clamp(confidence, 0.0f, 1.0f);
}

void MemoryService::mergeSessionIntoLongTerm(unsigned long nowMs) {
  if (profile_.session.sessionStartMs == 0 || nowMs <= profile_.session.sessionStartMs) {
    return;
  }

  const uint32_t elapsedMs = static_cast<uint32_t>(nowMs - profile_.session.sessionStartMs);
  if (elapsedMs > lastMergedSessionDurationMs_) {
    const uint32_t delta = elapsedMs - lastMergedSessionDurationMs_;
    profile_.longTerm.cumulativeSessionDurationMs = saturatingAdd(profile_.longTerm.cumulativeSessionDurationMs, delta);
    lastMergedSessionDurationMs_ = elapsedMs;
    dirtyLongTerm_ = true;
  }

  if (profile_.longTerm.totalSessions > 0U) {
    const uint64_t scaled = static_cast<uint64_t>(profile_.longTerm.totalInteractions) * 100ULL;
    profile_.longTerm.avgInteractionsPerSessionX100 =
        static_cast<uint32_t>(scaled / static_cast<uint64_t>(profile_.longTerm.totalSessions));
  } else {
    profile_.longTerm.avgInteractionsPerSessionX100 = 0;
  }
}

void MemoryService::publishPreference(unsigned long nowMs) {
  Event event;
  event.type = EventType::EVT_PREFERENCE_UPDATED;
  event.source = EventSource::MemoryService;
  event.value = static_cast<int>(profile_.longTerm.preferredFocus);
  event.timestamp = nowMs;
  eventBus_.publish(event);

  lastPublishedFocus_ = profile_.longTerm.preferredFocus;
  lastPublishedCalmStyle_ = profile_.longTerm.prefersCalmStyle;
}

void MemoryService::publishMemoryStatus(unsigned long nowMs) {
  Event event;
  event.type = EventType::EVT_MEMORY_UPDATED;
  event.source = EventSource::MemoryService;
  event.value = static_cast<int>(profile_.longTerm.totalInteractions > 2147483647U
                                     ? 2147483647U
                                     : profile_.longTerm.totalInteractions);
  event.timestamp = nowMs;
  eventBus_.publish(event);
}
