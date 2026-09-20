#pragma once

#include <cstdint>

struct NetworkConfig {
  const char *wifiSsid;
  const char *wifiPassword;
  const char *mqttHost;
  std::uint16_t mqttPort;
  const char *mqttUsername;
  const char *mqttPassword;
  const char *deviceId;
  const char *locationTopic;
  const char *statusTopic;
  std::uint32_t publishIntervalMs;
};
