#include "Payload.h"

#include <cmath>
#include <cstdio>

bool formatLocationPayload(const LocationFix &fix, const char *deviceId,
                           char *output, std::size_t outputSize) {
  if (!fix.valid || deviceId == nullptr || deviceId[0] == '\0' ||
      output == nullptr || outputSize == 0 || !std::isfinite(fix.latitude) ||
      !std::isfinite(fix.longitude) || !std::isfinite(fix.altitudeMeters) ||
      !std::isfinite(fix.speedKph) || !std::isfinite(fix.hdop) ||
      fix.latitude < -90.0 || fix.latitude > 90.0 || fix.longitude < -180.0 ||
      fix.longitude > 180.0 || fix.timestamp[0] == '\0') {
    return false;
  }

  const int written = std::snprintf(
      output, outputSize,
      "{\"device_id\":\"%s\",\"timestamp\":\"%s\",\"latitude\":%.6f,"
      "\"longitude\":%.6f,\"altitude_m\":%.1f,\"speed_kph\":%.1f,"
      "\"satellites\":%u,\"hdop\":%.1f,\"fix_age_ms\":%u}",
      deviceId, fix.timestamp, fix.latitude, fix.longitude,
      fix.altitudeMeters, fix.speedKph,
      static_cast<unsigned int>(fix.satellites), fix.hdop,
      static_cast<unsigned int>(fix.fixAgeMs));

  return written >= 0 && static_cast<std::size_t>(written) < outputSize;
}
