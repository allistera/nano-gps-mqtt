#include <cassert>
#include <cstdint>
#include <limits>

#include "PublishSchedule.h"

void runPublishScheduleTests() {
  PublishSchedule schedule(30000);
  assert(schedule.due(0));
  assert(schedule.due(12345));

  schedule.published(1000);
  assert(!schedule.due(1000));
  assert(!schedule.due(30999));
  assert(schedule.due(31000));
  assert(schedule.due(40000));

  schedule.published(31000);
  assert(!schedule.due(31000));
  assert(!schedule.due(60999));
  assert(schedule.due(61000));

  PublishSchedule rollover(30000);
  const std::uint32_t nearMax = std::numeric_limits<std::uint32_t>::max() - 100;
  rollover.published(nearMax);
  assert(!rollover.due(std::numeric_limits<std::uint32_t>::max()));
  assert(!rollover.due(29898));
  assert(rollover.due(29899));
}
