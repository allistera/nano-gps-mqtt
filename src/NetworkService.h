#pragma once

#include "LocationStore.h"
#include "NetworkConfig.h"
#include "PublishSchedule.h"
#include "RetryBackoff.h"

#include <MQTT.h>
#include <WiFi.h>

#include <cstddef>
#include <cstdint>

class NetworkService {
 public:
  NetworkService(const NetworkConfig &config, LocationStore &store);

  NetworkService(const NetworkService &) = delete;
  NetworkService &operator=(const NetworkService &) = delete;

  void begin();
  void poll(std::uint32_t nowMs);

 private:
  static constexpr std::uint32_t kInitialRetryMs = 1000;
  static constexpr std::uint32_t kMaximumRetryMs = 30000;
  static constexpr std::uint32_t kWifiAttemptTimeoutMs = 15000;
  static constexpr int kMqttKeepAliveSeconds = 15;
  static constexpr int kMqttTimeoutMs = 5000;
  static constexpr std::size_t kPayloadSize = 384;

  bool pollWifi(std::uint32_t nowMs);
  bool pollMqtt(std::uint32_t nowMs);
  void publishLocation(std::uint32_t nowMs);

  const NetworkConfig &config_;
  LocationStore &store_;
  WiFiClient wifiClient_;
  MQTTClient mqtt_;
  RetryBackoff wifiBackoff_;
  RetryBackoff mqttBackoff_;
  PublishSchedule schedule_;
  char clientId_[40] = {};
  char payload_[kPayloadSize] = {};
  bool wifiAttemptInProgress_ = false;
  std::uint32_t wifiAttemptStartedMs_ = 0;
  bool wifiWasConnected_ = false;
  bool mqttWasConnected_ = false;
};
