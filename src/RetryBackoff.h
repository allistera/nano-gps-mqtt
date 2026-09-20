#pragma once

#include <cstdint>

class RetryBackoff {
 public:
  RetryBackoff(std::uint32_t initialDelayMs, std::uint32_t maximumDelayMs)
      : initialDelayMs_(initialDelayMs),
        maximumDelayMs_(maximumDelayMs),
        nextDelayMs_(initialDelayMs) {}

  bool ready(std::uint32_t nowMs) const {
    return !waiting_ || nowMs - failedAtMs_ >= waitDelayMs_;
  }

  void failed(std::uint32_t nowMs) {
    failedAtMs_ = nowMs;
    waitDelayMs_ = nextDelayMs_;
    waiting_ = true;

    if (nextDelayMs_ >= maximumDelayMs_ ||
        nextDelayMs_ > maximumDelayMs_ - nextDelayMs_) {
      nextDelayMs_ = maximumDelayMs_;
    } else {
      nextDelayMs_ *= 2;
    }
  }

  void reset() {
    waiting_ = false;
    failedAtMs_ = 0;
    waitDelayMs_ = 0;
    nextDelayMs_ = initialDelayMs_;
  }

 private:
  std::uint32_t initialDelayMs_;
  std::uint32_t maximumDelayMs_;
  std::uint32_t nextDelayMs_;
  std::uint32_t failedAtMs_ = 0;
  std::uint32_t waitDelayMs_ = 0;
  bool waiting_ = false;
};
