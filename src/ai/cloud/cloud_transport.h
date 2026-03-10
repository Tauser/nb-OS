#pragma once

#include "../../models/cloud_types.h"

struct CloudTransportRequest {
  CloudRequestType type = CloudRequestType::None;
  unsigned long timeoutMs = 0;
  const char* authToken = nullptr;
};

struct CloudTransportResponse {
  CloudResultCode result = CloudResultCode::None;
  bool success = false;
  unsigned long latencyMs = 0;
};

class CloudTransport {
public:
  void init();
  bool begin(const CloudTransportRequest& request, unsigned long nowMs);
  bool poll(unsigned long nowMs, CloudTransportResponse& outResponse);
  void cancel();
  bool busy() const;

private:
  CloudResultCode evaluateOutcome(const CloudTransportRequest& request) const;

  bool busy_ = false;
  CloudTransportRequest request_{};
  CloudResultCode plannedResult_ = CloudResultCode::None;
  unsigned long startMs_ = 0;
  unsigned long completeAtMs_ = 0;
};
