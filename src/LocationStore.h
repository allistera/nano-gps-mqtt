#pragma once

#include "LocationFix.h"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

class LocationStore {
 public:
  LocationStore();
  ~LocationStore();

  LocationStore(const LocationStore &) = delete;
  LocationStore &operator=(const LocationStore &) = delete;

  bool read(LocationFix &destination) const;
  bool write(const LocationFix &source);

 private:
  static constexpr TickType_t kMutexWaitTicks = pdMS_TO_TICKS(10);

  mutable SemaphoreHandle_t mutex_ = nullptr;
  LocationFix fix_{};
};
