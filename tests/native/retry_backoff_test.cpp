#include <cassert>
#include <cstdint>
#include <limits>

#include "RetryBackoff.h"

void runRetryBackoffTests() {
  RetryBackoff retry(1000, 30000);
  assert(retry.ready(0));

  std::uint32_t now = 100;
  const std::uint32_t delays[] = {1000, 2000, 4000, 8000,
                                  16000, 30000, 30000};
  for (const std::uint32_t delay : delays) {
    retry.failed(now);
    assert(!retry.ready(now + delay - 1));
    now += delay;
    assert(retry.ready(now));
  }

  retry.reset();
  assert(retry.ready(now));
  retry.failed(now);
  assert(!retry.ready(now + 999));
  assert(retry.ready(now + 1000));

  RetryBackoff rolloverRetry(1000, 30000);
  rolloverRetry.failed(std::numeric_limits<std::uint32_t>::max() - 500);
  assert(!rolloverRetry.ready(std::numeric_limits<std::uint32_t>::max() - 1));
  assert(rolloverRetry.ready(499));
}
