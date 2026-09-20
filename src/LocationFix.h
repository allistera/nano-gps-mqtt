#pragma once

#include <cstdint>

struct LocationFix {
  bool valid;
  double latitude;
  double longitude;
  double altitudeMeters;
  double speedKph;
  std::uint8_t satellites;
  double hdop;
  std::uint32_t fixAgeMs;
  char timestamp[21];
};
