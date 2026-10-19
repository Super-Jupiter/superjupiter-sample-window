// SPDX-License-Identifier: MIT
// Copyright (c) 2026 SuperJupiter
#ifndef SUPERJUPITER_SAMPLE_WINDOW_H
#define SUPERJUPITER_SAMPLE_WINDOW_H

#if defined(__FAST_MATH__) ||                                                  \
    (defined(__FINITE_MATH_ONLY__) && __FINITE_MATH_ONLY__ > 0)
#error                                                                         \
    "SuperJupiter SampleWindow requires IEEE non-finite checks; disable fast-math/finite-math-only."
#endif

#define SUPERJUPITER_SAMPLE_WINDOW_VERSION "0.1.1"

#include <float.h>
#include <stddef.h>
#include <stdint.h>

namespace superjupiter {
namespace sample_window {

namespace detail {
inline bool finiteFloat(float value) {
  return value == value && value >= -FLT_MAX && value <= FLT_MAX;
}
inline bool finiteDouble(double value) {
  return value == value && value >= -DBL_MAX && value <= DBL_MAX;
}
inline uint32_t saturatingIncrement(uint32_t value) {
  return value == UINT32_MAX ? value : value + 1U;
}
} // namespace detail

enum class InputError : uint8_t {
  None,
  InvalidConfiguration,
  InvalidTimestamp
};

struct Config {
  uint32_t expectedIntervalMs = 0; // Zero disables gap detection.
  uint32_t gapToleranceMs = 0;
};

struct Sample {
  float value = 0.0f; // Raw value, including NaN, infinity and signed zero.
  uint32_t sampledAtMs = 0;
  uint32_t intervalMs = 0; // Since previous accepted entry, possibly evicted.
  bool finite() const { return detail::finiteFloat(value); }
};

struct PushResult {
  bool accepted = false;
  bool overwroteOldest = false;
  bool finite = false;
  InputError error = InputError::None;
};

struct Summary {
  size_t storedCount = 0;
  size_t finiteCount = 0;
  size_t invalidCount = 0;
  bool full = false;
  bool configured = true;
  bool hasStatistics = false;
  bool numericError = false;
  bool hasGap = false;
  uint32_t gapCount = 0;
  uint32_t oldestAtMs = 0;
  uint32_t latestAtMs = 0;
  uint64_t spanMs =
      0; // Sum of retained intervals; can span multiple rollovers.
  uint32_t largestIntervalMs = 0;
  uint32_t overwrittenSinceReset = 0; // Saturating count, not lost sensor data.
  float minimum = 0.0f;
  float maximum = 0.0f;
  double mean = 0.0;
  double populationVariance = 0.0;
};

// One scalar stream, caller-owned capture clock, no heap/clock reads/locking.
// Capacity bounds make the maximum retained-interval sum fit in uint64_t.
template <size_t Capacity> class SampleWindow {
  static_assert(Capacity > 0,
                "SuperJupiter SampleWindow requires positive capacity");
  static_assert(Capacity <= 65535U,
                "SuperJupiter SampleWindow capacity exceeds 65535");

public:
  explicit SampleWindow(const Config &config = Config()) : config_(config) {
    reset();
  }

  static bool validConfig(const Config &config) {
    return config.expectedIntervalMs <= maxInterval() &&
           config.gapToleranceMs <= maxInterval() - config.expectedIntervalMs &&
           (config.expectedIntervalMs != 0 || config.gapToleranceMs == 0);
  }
  bool configured() const { return configured_; }
  const Config &config() const { return config_; }
  size_t count() const { return count_; }
  static size_t capacity() { return Capacity; }
  bool full() const { return count_ == Capacity; }
  uint32_t overwrittenSinceReset() const { return overwritten_; }

  // Logical reset, not secure memory erasure; configuration is retained.
  void reset() {
    configured_ = validConfig(config_);
    first_ = count_ = 0;
    lastAtMs_ = overwritten_ = 0;
  }

  PushResult push(float value, uint32_t sampledAtMs) {
    PushResult result;
    if (!configured_) {
      result.error = InputError::InvalidConfiguration;
      return result;
    }
    uint32_t interval = 0;
    if (count_ != 0) {
      interval = sampledAtMs - lastAtMs_;
      if (interval == 0 || interval > maxInterval()) {
        result.error = InputError::InvalidTimestamp;
        return result;
      }
    }
    const bool overwrites = full();
    const size_t slot = overwrites ? first_ : physicalIndex(count_);
    samples_[slot].value = value;
    samples_[slot].sampledAtMs = sampledAtMs;
    samples_[slot].intervalMs = interval;
    if (overwrites) {
      first_ = first_ + 1 == Capacity ? 0 : first_ + 1;
      overwritten_ = detail::saturatingIncrement(overwritten_);
    } else {
      ++count_;
    }
    lastAtMs_ = sampledAtMs;
    result.accepted = true;
    result.overwroteOldest = overwrites;
    result.finite = detail::finiteFloat(value);
    return result;
  }

  // Chronological index, zero is oldest. On failure output is unchanged.
  bool sampleAt(size_t index, Sample &output) const {
    if (index >= count_)
      return false;
    output = samples_[physicalIndex(index)];
    return true;
  }

  // O(count), sample-weighted population statistics over finite retained
  // values. Non-finite arithmetic invalidates statistics; counts/timing remain
  // available.
  Summary summary() const {
    Summary result;
    result.configured = configured_;
    result.storedCount = count_;
    result.full = full();
    result.overwrittenSinceReset = overwritten_;
    if (count_ == 0)
      return result;
    result.oldestAtMs = samples_[first_].sampledAtMs;
    result.latestAtMs = lastAtMs_;
    double mean = 0.0;
    double m2 = 0.0;
    for (size_t i = 0; i < count_; ++i) {
      const Sample &sample = samples_[physicalIndex(i)];
      // The first entry's incoming interval starts outside the retained window.
      if (i != 0) {
        result.spanMs += sample.intervalMs;
        if (sample.intervalMs > result.largestIntervalMs)
          result.largestIntervalMs = sample.intervalMs;
        if (config_.expectedIntervalMs != 0 &&
            sample.intervalMs >
                config_.expectedIntervalMs + config_.gapToleranceMs) {
          ++result.gapCount;
        }
      }
      if (!sample.finite()) {
        ++result.invalidCount;
        continue;
      }
      ++result.finiteCount;
      if (result.finiteCount == 1) {
        result.minimum = result.maximum = sample.value;
        mean = sample.value;
        continue;
      }
      if (sample.value < result.minimum)
        result.minimum = sample.value;
      if (sample.value > result.maximum)
        result.maximum = sample.value;
      if (!result.numericError) {
        const double value = sample.value;
        const double delta = value - mean;
        mean += delta / static_cast<double>(result.finiteCount);
        const double deltaAfterMean = value - mean;
        m2 += delta * deltaAfterMean;
        if (!detail::finiteDouble(delta) || !detail::finiteDouble(mean) ||
            !detail::finiteDouble(m2) || m2 < 0.0)
          result.numericError = true;
      }
    }
    result.hasGap = result.gapCount != 0;
    if (result.finiteCount != 0 && !result.numericError) {
      result.mean = mean;
      result.populationVariance = m2 / static_cast<double>(result.finiteCount);
      result.hasStatistics = detail::finiteDouble(result.populationVariance);
      result.numericError = !result.hasStatistics;
    }
    if (result.numericError) {
      result.minimum = result.maximum = 0.0f;
      result.mean = result.populationVariance = 0.0;
    }
    return result;
  }

private:
  static uint32_t maxInterval() { return 0x7fffffffUL; }
  size_t physicalIndex(size_t offset) const {
    // Avoid overflowing 16-bit size_t on AVR before applying ring arithmetic.
    const size_t untilEnd = Capacity - first_;
    return offset < untilEnd ? first_ + offset : offset - untilEnd;
  }

  Config config_;
  Sample samples_[Capacity];
  size_t first_ = 0;
  size_t count_ = 0;
  uint32_t lastAtMs_ = 0;
  uint32_t overwritten_ = 0;
  bool configured_ = true;
};

} // namespace sample_window
} // namespace superjupiter
#endif
