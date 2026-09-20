#include <Arduino.h>

#include <GpsService.h>
#include <LocationFix.h>
#include <LocationStore.h>
#include <NetworkConfig.h>
#include <NetworkService.h>

#include "secrets.h"

namespace {

constexpr char kDeviceId[] = "nano-gps-01";
constexpr NetworkConfig kNetworkConfig{WIFI_SSID,
                                       WIFI_PASSWORD,
                                       "mqtt.allisterantosik.com",
                                       1883,
                                       MQTT_USERNAME,
                                       MQTT_PASSWORD,
                                       kDeviceId,
                                       "devices/nano-gps-01/location",
                                       "devices/nano-gps-01/status",
                                       30000};

constexpr uint32_t kGpsBaud = 9600;
constexpr uint32_t kGpsTaskStackBytes = 4096;
constexpr uint32_t kNetworkTaskStackBytes = 8192;
constexpr uint32_t kDiagnosticIntervalMs = 10000;

LocationStore locationStore;
GpsService gpsService(locationStore);
NetworkService networkService(kNetworkConfig, locationStore);

void gpsTask(void *) {
  for (;;) {
    gpsService.poll(Serial1, millis());
    vTaskDelay(1);
  }
}

void networkTask(void *) {
  for (;;) {
    networkService.poll(millis());
    vTaskDelay(10);
  }
}

[[noreturn]] void haltStartup(const char *reason) {
  Serial.print("startup: ");
  Serial.println(reason);
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("startup: nano-gps-mqtt");

  Serial1.begin(kGpsBaud, SERIAL_8N1, D6, D7);
  networkService.begin();

  if (xTaskCreate(gpsTask, "gps", kGpsTaskStackBytes, nullptr, 2, nullptr) !=
      pdPASS) {
    haltStartup("gps task creation failed");
  }
  if (xTaskCreate(networkTask, "network", kNetworkTaskStackBytes, nullptr, 1,
                  nullptr) != pdPASS) {
    haltStartup("network task creation failed");
  }
  Serial.println("startup: tasks running");
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(kDiagnosticIntervalMs));

  LocationFix fix{};
  const bool readable = locationStore.read(fix);
  Serial.printf("gps: %lu bytes received, ",
                static_cast<unsigned long>(gpsService.bytesReceived()));
  if (readable && fix.valid) {
    Serial.printf("fix %s, %u satellites\n", fix.timestamp,
                  static_cast<unsigned int>(fix.satellites));
  } else {
    Serial.println("no valid fix yet");
  }
}
