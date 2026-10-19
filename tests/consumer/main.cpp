// SPDX-License-Identifier: MIT
#include <SuperJupiterSampleWindow.h>
int main() {
  superjupiter::sample_window::SampleWindow<2> samples;
  samples.push(2.0f, 0);
  samples.push(4.0f, 100);
  return samples.summary().mean == 3.0 ? 0 : 1;
}
