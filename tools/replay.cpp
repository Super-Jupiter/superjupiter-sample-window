// SPDX-License-Identifier: MIT
#include <SuperJupiterSampleWindow.h>
#include <iostream>
#include <limits>

int main() {
  superjupiter::sample_window::Config c;
  c.expectedIntervalMs = 100;
  superjupiter::sample_window::SampleWindow<4> samples(c);
  const float values[] = {20, 21, std::numeric_limits<float>::quiet_NaN(), 22, 23};
  const uint32_t times[] = {0, 100, 200, 600, 700};
  std::cout << "SuperJupiter SampleWindow: synthetic data only\n";
  for (unsigned i = 0; i < 5; ++i) {
    samples.push(values[i], times[i]);
    const superjupiter::sample_window::Summary s = samples.summary();
    std::cout << "count=" << s.storedCount << " invalid=" << s.invalidCount
      << " gaps=" << s.gapCount << " overwritten=" << s.overwrittenSinceReset;
    if (s.hasStatistics) std::cout << " mean=" << s.mean;
    std::cout << '\n';
  }
}
