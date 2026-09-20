#include <cassert>
#include <cmath>
#include <cstring>
#include <limits>

#include "LocationFix.h"
#include "Payload.h"

void runPayloadTests() {
  LocationFix fix{true,
                  55.953251,
                  -3.188267,
                  72.4,
                  0.0,
                  8,
                  1.1,
                  240,
                  "2026-09-20T14:32:10Z"};
  char payload[256];

  assert(formatLocationPayload(fix, "nano-gps-01", payload, sizeof(payload)));
  assert(std::strstr(payload, "\"device_id\":\"nano-gps-01\"") != nullptr);
  assert(std::strstr(payload, "\"latitude\":55.953251") != nullptr);
  assert(std::strstr(payload, "\"longitude\":-3.188267") != nullptr);
  assert(std::strstr(payload, "\"satellites\":8") != nullptr);

  fix.valid = false;
  assert(!formatLocationPayload(fix, "nano-gps-01", payload, sizeof(payload)));
  fix.valid = true;

  fix.latitude = 90.000001;
  assert(!formatLocationPayload(fix, "nano-gps-01", payload, sizeof(payload)));
  fix.latitude = 55.953251;
  fix.longitude = -180.000001;
  assert(!formatLocationPayload(fix, "nano-gps-01", payload, sizeof(payload)));
  fix.longitude = -3.188267;

  fix.latitude = std::numeric_limits<double>::quiet_NaN();
  assert(!formatLocationPayload(fix, "nano-gps-01", payload, sizeof(payload)));
  fix.latitude = 55.953251;
  fix.altitudeMeters = std::numeric_limits<double>::infinity();
  assert(!formatLocationPayload(fix, "nano-gps-01", payload, sizeof(payload)));
  fix.altitudeMeters = 72.4;

  assert(!formatLocationPayload(fix, "", payload, sizeof(payload)));
  char tinyPayload[8];
  assert(!formatLocationPayload(fix, "nano-gps-01", tinyPayload,
                                sizeof(tinyPayload)));
}
