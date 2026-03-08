#include "dialogue_engine.h"

#include "../../config/hardware_config.h"

bool DialogueEngine::init() {
  return true;
}

DialogueReply DialogueEngine::buildReply(LocalIntent intent) const {
  DialogueReply reply;

  switch (intent) {
    case LocalIntent::Hello:
      reply.text = "ola";
      reply.toneHz = HardwareConfig::Dialogue::HELLO_TONE_HZ;
      reply.toneMs = HardwareConfig::Dialogue::HELLO_TONE_MS;
      break;

    case LocalIntent::Status:
      reply.text = "status ok";
      reply.toneHz = HardwareConfig::Dialogue::STATUS_TONE_HZ;
      reply.toneMs = HardwareConfig::Dialogue::STATUS_TONE_MS;
      break;

    case LocalIntent::Sleep:
      reply.text = "indo dormir";
      reply.toneHz = HardwareConfig::Dialogue::SLEEP_TONE_HZ;
      reply.toneMs = HardwareConfig::Dialogue::SLEEP_TONE_MS;
      break;

    case LocalIntent::Wake:
      reply.text = "acordando";
      reply.toneHz = HardwareConfig::Dialogue::WAKE_TONE_HZ;
      reply.toneMs = HardwareConfig::Dialogue::WAKE_TONE_MS;
      break;

    case LocalIntent::Photo:
      reply.text = "foto";
      reply.toneHz = HardwareConfig::Dialogue::PHOTO_TONE_HZ;
      reply.toneMs = HardwareConfig::Dialogue::PHOTO_TONE_MS;
      break;

    default:
      break;
  }

  return reply;
}
