#pragma once

#include "../../models/intent_types.h"

class DialogueEngine {
public:
  bool init();
  DialogueReply buildReply(LocalIntent intent) const;
};
