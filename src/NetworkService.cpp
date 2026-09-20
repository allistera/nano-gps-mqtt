#include "NetworkService.h"

#include "Payload.h"

#include <Arduino.h>

#include <cstdio>

NetworkService::NetworkService(const NetworkConfig &config,
                               LocationStore &store)
    : config_(config),
      store_(store),
      mqtt_(static_cast<int>(kPayloadSize)),
      wifiBackoff_(kInitialRetryMs, kMaximumRetryMs),
      mqttBackoff_(kInitialRetryMs, kMaximumRetryMs),
      schedule_(config.publishIntervalMs) {}

void NetworkService::begin() {
  const std::uint64_t mac = ESP.getEfuseMac();
  std::snprintf(clientId_, sizeof(clientId_), "%s-%012llX", config_.deviceId,
                static_cast<unsigned long long>(mac));

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(false);

  mqtt_.begin(config_.mqttHost, config_.mqttPort, wifiClient_);
  mqtt_.setOptions(kMqttKeepAliveSeconds, true, kMqttTimeoutMs);

  Serial.println("network: started");
}

void NetworkService::poll(std::uint32_t nowMs) {
  if (!pollWifi(nowMs)) {
    return;
  }
  if (!pollMqtt(nowMs)) {
    return;
  }
  publishLocation(nowMs);
}

bool NetworkService::pollWifi(std::uint32_t nowMs) {
  const bool connected = WiFi.status() == WL_CONNECTED;

  if (connected) {
    if (!wifiWasConnected_) {
      Serial.println("network: wifi connected");
      wifiWasConnected_ = true;
      wifiAttemptInProgress_ = false;
      wifiBackoff_.reset();
    }
    return true;
  }

  if (wifiWasConnected_) {
    Serial.println("network: wifi lost");
    wifiWasConnected_ = false;
    mqttWasConnected_ = false;
  }

  if (wifiAttemptInProgress_) {
    if (nowMs - wifiAttemptStartedMs_ >= kWifiAttemptTimeoutMs) {
      Serial.printf("network: wifi attempt failed, status %d\n",
                    static_cast<int>(WiFi.status()));
      WiFi.disconnect();
      wifiAttemptInProgress_ = false;
      wifiBackoff_.failed(nowMs);
    }
    return false;
  }

  if (wifiBackoff_.ready(nowMs)) {
    Serial.println("network: wifi connecting");
    WiFi.begin(config_.wifiSsid, config_.wifiPassword);
    wifiAttemptInProgress_ = true;
    wifiAttemptStartedMs_ = nowMs;
  }
  return false;
}

bool NetworkService::pollMqtt(std::uint32_t nowMs) {
  if (mqtt_.connected()) {
    mqtt_.loop();
    if (mqtt_.connected()) {
      return true;
    }
  }

  if (mqttWasConnected_) {
    Serial.printf("network: mqtt lost, error %d\n",
                  static_cast<int>(mqtt_.lastError()));
    mqttWasConnected_ = false;
  }

  if (!mqttBackoff_.ready(nowMs)) {
    return false;
  }

  Serial.println("network: mqtt connecting");
  mqtt_.setWill(config_.statusTopic, "offline", true, 1);
  if (!mqtt_.connect(clientId_, config_.mqttUsername, config_.mqttPassword)) {
    Serial.printf("network: mqtt connect failed, error %d, return code %d\n",
                  static_cast<int>(mqtt_.lastError()),
                  static_cast<int>(mqtt_.returnCode()));
    mqttBackoff_.failed(nowMs);
    return false;
  }

  if (!mqtt_.publish(config_.statusTopic, "online", true, 1)) {
    Serial.printf("network: status publish failed, error %d\n",
                  static_cast<int>(mqtt_.lastError()));
    mqttBackoff_.failed(nowMs);
    return false;
  }

  Serial.println("network: mqtt connected");
  mqttWasConnected_ = true;
  mqttBackoff_.reset();
  return true;
}

void NetworkService::publishLocation(std::uint32_t nowMs) {
  if (!schedule_.due(nowMs)) {
    return;
  }

  LocationFix fix{};
  if (!store_.read(fix)) {
    Serial.println("network: location store busy");
    return;
  }
  if (!fix.valid) {
    return;
  }
  if (!formatLocationPayload(fix, config_.deviceId, payload_,
                             sizeof(payload_))) {
    Serial.println("network: location payload rejected");
    return;
  }

  if (!mqtt_.publish(config_.locationTopic, payload_, false, 1)) {
    Serial.printf("network: location publish failed, error %d\n",
                  static_cast<int>(mqtt_.lastError()));
    return;
  }

  Serial.println("network: location published");
  schedule_.published(nowMs);
}
