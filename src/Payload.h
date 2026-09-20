#pragma once

#include <cstddef>

#include "LocationFix.h"

bool formatLocationPayload(const LocationFix &fix, const char *deviceId,
                           char *output, std::size_t outputSize);
