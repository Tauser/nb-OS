#pragma once

#include "../../models/cloud_types.h"
#include "../../models/power_state.h"

class CloudPolicy {
public:
  void init();

  bool canStart(bool cloudEnabled, PowerMode powerMode, unsigned long nowMs, CloudResultCode& outReason) const;
  unsigned long timeoutFor(CloudRequestType type) const;
  bool shouldRetry(CloudResultCode failure, uint8_t attempt) const;
  unsigned long retryDelayMs(uint8_t attempt) const;

  void onAttemptFailure(CloudResultCode failure, unsigned long nowMs);
  void onFinalResult(CloudResultCode result, unsigned long nowMs);

private:
  bool isCircuitOpen(unsigned long nowMs) const;

  uint8_t consecutiveFailures_ = 0;
  unsigned long circuitOpenUntilMs_ = 0;
};
