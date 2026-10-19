// SPDX-License-Identifier: MIT
namespace superjupiter {
struct Config {};
struct Result {};
namespace another_component { struct Config {}; struct Summary {}; }
}
#include <SuperJupiterSampleWindow.h>
int main() {
  superjupiter::sample_window::SampleWindow<4> samples;
  samples.push(1.0f, 0);
  return samples.summary().mean == 1.0 ? 0 : 1;
}
