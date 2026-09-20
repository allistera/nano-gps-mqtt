#pragma once

#include "LocationStore.h"

#include <Arduino.h>
#include <TinyGPSPlus.h>

#include <cstdint>

class GpsService {
 public:
  explicit GpsService(LocationStore &store) : store_(store) {}

  void poll(Stream &stream, std::uint32_t nowMs);
  std::uint32_t bytesReceived() const { return bytesReceived_; }

 private:
  LocationStore &store_;
  TinyGPSPlus gps_;
  std::uint32_t bytesReceived_ = 0;
};
