#pragma once

#include <cstdint>

class PublishSchedule {
 public:
  explicit PublishSchedule(std::uint32_t intervalMs) : intervalMs_(intervalMs) {}

  bool due(std::uint32_t nowMs) const {
    return !published_ || nowMs - lastPublishedMs_ >= intervalMs_;
  }

  void published(std::uint32_t nowMs) {
    published_ = true;
    lastPublishedMs_ = nowMs;
  }

 private:
  std::uint32_t intervalMs_;
  std::uint32_t lastPublishedMs_ = 0;
  bool published_ = false;
};
