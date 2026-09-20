#include "GpsService.h"

#include <cstdio>

void GpsService::poll(Stream &stream, std::uint32_t) {
  while (stream.available() > 0) {
    const int value = stream.read();
    if (value >= 0) {
      gps_.encode(static_cast<char>(value));
      ++bytesReceived_;
    }
  }

  if (!gps_.location.isUpdated() || !gps_.location.isValid() ||
      !gps_.date.isValid() || !gps_.time.isValid()) {
    return;
  }

  const unsigned int year = gps_.date.year();
  const unsigned int month = gps_.date.month();
  const unsigned int day = gps_.date.day();
  const unsigned int hour = gps_.time.hour();
  const unsigned int minute = gps_.time.minute();
  const unsigned int second = gps_.time.second();
  if (year > 9999 || month < 1 || month > 12 || day < 1 || day > 31 ||
      hour > 23 || minute > 59 || second > 59) {
    return;
  }

  LocationFix fix{};
  fix.valid = true;
  fix.latitude = gps_.location.lat();
  fix.longitude = gps_.location.lng();
  fix.altitudeMeters = gps_.altitude.isValid() ? gps_.altitude.meters() : 0.0;
  fix.speedKph = gps_.speed.isValid() ? gps_.speed.kmph() : 0.0;
  fix.satellites = gps_.satellites.isValid()
                       ? static_cast<std::uint8_t>(gps_.satellites.value())
                       : 0;
  fix.hdop = gps_.hdop.isValid() ? gps_.hdop.hdop() : 0.0;
  fix.fixAgeMs = static_cast<std::uint32_t>(gps_.location.age());

  std::snprintf(fix.timestamp, sizeof(fix.timestamp),
                "%04u-%02u-%02uT%02u:%02u:%02uZ", year, month, day, hour,
                minute, second);

  store_.write(fix);
}
