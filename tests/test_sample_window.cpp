// SPDX-License-Identifier: MIT
#include <SuperJupiterSampleWindow.h>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include "reference_fixture.h"

using namespace superjupiter::sample_window;
static unsigned checks = 0;
static unsigned allocations = 0;
void* operator new(std::size_t size) {
  ++allocations;
  if (void* p = std::malloc(size)) return p;
  throw std::bad_alloc();
}
void operator delete(void* p) noexcept { std::free(p); }
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete[](void* p) noexcept { ::operator delete(p); }
#define CHECK(expr) do { ++checks; if (!(expr)) { std::cerr << __LINE__ << ": " #expr "\n"; std::exit(1); } } while (false)

static void initial_reset_and_bounds() {
  SampleWindow<2> w;
  CHECK(w.configured());
  CHECK(w.count() == 0);
  CHECK(w.capacity() == 2);
  CHECK(!w.full());
  Summary s = w.summary();
  CHECK(s.configured && !s.hasStatistics && s.storedCount == 0);
  CHECK(s.spanMs == 0 && !s.hasGap);
  Sample output;
  output.value = 77;
  CHECK(!w.sampleAt(0, output));
  CHECK(output.value == 77);
  CHECK(w.push(4, 0).accepted);
  CHECK(w.sampleAt(0, output));
  CHECK(output.value == 4 && output.sampledAtMs == 0 && output.intervalMs == 0);
  s = w.summary();
  CHECK(s.hasStatistics && s.mean == 4 && s.populationVariance == 0);
  w.push(6, 100);
  CHECK(w.full());
  CHECK(!w.sampleAt(2, output));
  CHECK(output.value == 4);  // A failed lookup does not mutate output.
  PushResult pushed = w.push(8, 200);
  CHECK(pushed.accepted && pushed.overwroteOldest && pushed.finite);
  CHECK(w.overwrittenSinceReset() == 1);
  CHECK(w.sampleAt(0, output) && output.value == 6);
  CHECK(w.sampleAt(1, output) && output.value == 8);
  s = w.summary();
  CHECK(s.mean == 7 && s.populationVariance == 1);
  CHECK(s.oldestAtMs == 100 && s.latestAtMs == 200 && s.spanMs == 100);
  w.reset();
  CHECK(w.count() == 0 && w.overwrittenSinceReset() == 0);
  CHECK(w.push(3, 0).accepted);
}

static void invalid_values_and_single_capacity() {
  SampleWindow<1> one;
  one.push(-0.0f, 100);
  Sample raw;
  CHECK(one.sampleAt(0, raw) && std::signbit(raw.value));
  CHECK(one.summary().hasStatistics);
  const float invalid[] = {std::numeric_limits<float>::quiet_NaN(),
    std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity()};
  for (unsigned i = 0; i < 3; ++i) {
    PushResult r = one.push(invalid[i], 200 + i);
    CHECK(r.accepted && !r.finite && r.overwroteOldest && r.error == InputError::None);
    CHECK(one.sampleAt(0, raw) && !raw.finite());
    CHECK((std::isnan(invalid[i]) && std::isnan(raw.value)) || invalid[i] == raw.value);
    const Summary s = one.summary();
    CHECK(s.storedCount == 1 && s.invalidCount == 1 && s.finiteCount == 0);
    CHECK(!s.hasStatistics && !s.numericError);
    CHECK(s.spanMs == 0 && s.largestIntervalMs == 0 && !s.hasGap);
  }
  SampleWindow<5> mixed;
  mixed.push(2, 0);
  mixed.push(invalid[0], 100);
  mixed.push(4, 200);
  mixed.push(invalid[1], 300);
  Summary s = mixed.summary();
  CHECK(s.finiteCount == 2 && s.invalidCount == 2);
  CHECK(s.minimum == 2 && s.maximum == 4 && s.mean == 3 && s.populationVariance == 1);
  CHECK(s.spanMs == 300);
}

static void timestamp_contract_and_gaps() {
  Config c;
  c.expectedIntervalMs = 100;
  c.gapToleranceMs = 20;
  SampleWindow<3> w(c);
  w.push(1, 100);
  CHECK(w.push(2, 100).error == InputError::InvalidTimestamp);
  CHECK(!w.push(2, 99).accepted);
  CHECK(w.count() == 1 && w.summary().latestAtMs == 100);
  w.push(2, 220);
  CHECK(!w.summary().hasGap);
  w.push(3, 341);
  CHECK(w.summary().hasGap && w.summary().gapCount == 1);
  CHECK(w.summary().largestIntervalMs == 121 && w.summary().spanMs == 241);
  w.push(4, 441);
  CHECK(w.summary().hasGap);
  w.push(5, 541);
  CHECK(!w.summary().hasGap);  // Gap entering oldest is now outside the window.
  CHECK(w.summary().spanMs == 200);
  Sample first;
  CHECK(w.sampleAt(0, first) && first.intervalMs == 121);
  CHECK(!w.push(6, 541).overwroteOldest);
  CHECK(w.overwrittenSinceReset() == 2);
  w.reset();
  w.push(1, 0xfffffff0UL);
  CHECK(w.push(2, 0x54).accepted);
  CHECK(w.summary().spanMs == 100);
  w.reset();
  w.push(1, 0);
  CHECK(w.push(2, 0x7fffffffUL).accepted);
  CHECK(!w.push(3, 0xffffffffUL).accepted);  // Exactly half range is unsupported.
  CHECK(w.count() == 2);
}

static void multiple_rollovers_and_configuration() {
  SampleWindow<8> w;
  uint32_t at = 0;
  w.push(0, at);
  for (unsigned i = 1; i < 8; ++i) {
    at += 1000000000UL;
    CHECK(w.push(static_cast<float>(i), at).accepted);
  }
  CHECK(w.summary().spanMs == 7000000000ULL);
  CHECK(w.summary().oldestAtMs == 0 && w.summary().latestAtMs == at);
  CHECK(w.summary().largestIntervalMs == 1000000000UL);
  Config c;
  c.gapToleranceMs = 1;
  CHECK(!SampleWindow<2>::validConfig(c));
  SampleWindow<2> invalid(c);
  CHECK(!invalid.configured());
  CHECK(invalid.push(1, 0).error == InputError::InvalidConfiguration);
  CHECK(!invalid.summary().configured && invalid.count() == 0);
  c.expectedIntervalMs = 0x7fffffffUL;
  CHECK(!SampleWindow<2>::validConfig(c));
  c.gapToleranceMs = 0;
  CHECK(SampleWindow<2>::validConfig(c));
  c.expectedIntervalMs = 0x80000000UL;
  CHECK(!SampleWindow<2>::validConfig(c));
  CHECK(detail::saturatingIncrement(UINT32_MAX - 1) == UINT32_MAX);
  CHECK(detail::saturatingIncrement(UINT32_MAX) == UINT32_MAX);
}

template <size_t Capacity>
static void reference_cases() {
  for (size_t i = 0; i < referenceCaseCount; ++i) {
    const ReferenceCase& f = referenceCases[i];
    if (f.capacity != Capacity) continue;
    SampleWindow<Capacity> w;
    for (size_t k = 0; k < f.inputCount; ++k) w.push(f.values[k], static_cast<uint32_t>(k * 100));
    Summary s = w.summary();
    CHECK(s.hasStatistics && !s.numericError);
    CHECK(s.finiteCount == f.outputCount);
    CHECK(s.minimum == f.minimum && s.maximum == f.maximum);
    CHECK(std::fabs(s.mean - f.mean) <= 1e-10 * (1 + std::fabs(f.mean)));
    CHECK(std::fabs(s.populationVariance - f.variance) <= 1e-10 * (1 + std::fabs(f.variance)));
  }
}

static void extremes_and_no_allocation() {
  SampleWindow<2> w;
  const float large = std::numeric_limits<float>::max();
  w.push(-large, 0);
  w.push(large, 1);
  Summary s = w.summary();
  // Hosts with wider double can represent this; float-width double must report it.
  if (std::numeric_limits<double>::max_exponent > std::numeric_limits<float>::max_exponent * 2) {
    CHECK(s.hasStatistics && !s.numericError && std::isfinite(s.populationVariance));
  } else {
    CHECK(!s.hasStatistics && s.numericError);
  }
  const unsigned before = allocations;
  SampleWindow<600> stream;
  SampleWindow<2> separate;
  for (uint32_t i = 0; i < 100000; ++i) {
    const PushResult r = stream.push(static_cast<float>(i % 20), i * 100);
    CHECK(r.accepted && r.finite);
  }
  s = stream.summary();
  CHECK(s.storedCount == 600 && s.spanMs == 59900 && s.full);
  CHECK(s.overwrittenSinceReset == 99400);
  CHECK(std::fabs(s.mean - 9.5) < 1e-12 && std::fabs(s.populationVariance - 33.25) < 1e-10);
  CHECK(separate.count() == 0);
  Sample raw;
  CHECK(stream.sampleAt(0, raw) && raw.sampledAtMs == 9940000);
  CHECK(allocations == before);
}

int main() {
  initial_reset_and_bounds();
  invalid_values_and_single_capacity();
  timestamp_contract_and_gaps();
  multiple_rollovers_and_configuration();
  reference_cases<3>();
  reference_cases<8>();
  extremes_and_no_allocation();
  std::cout << "PASS: " << checks << " checks; SampleWindow<16>: "
            << sizeof(SampleWindow<16>) << " host bytes; hardware validation pending\n";
}
