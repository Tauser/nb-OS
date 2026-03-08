#pragma once

#include "long_term_memory.h"
#include "preference_memory.h"
#include "session_memory.h"
#include "short_term_memory.h"

struct MemoryProfile {
  ShortTermMemory shortTerm;
  SessionMemory session;
  LongTermMemory longTerm;
  PreferenceMemory preference;
};
