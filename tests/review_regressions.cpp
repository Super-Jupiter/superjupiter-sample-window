// SPDX-License-Identifier: MIT
#include <SuperJupiterSampleWindow.h>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
static unsigned checks = 0;
#define CHECK(x)                                                               \
  do {                                                                         \
    ++checks;                                                                  \
    if (!(x)) {                                                                \
      std::fprintf(stderr, "FAIL %s:%d %s\n", __FILE__, __LINE__, #x);         \
      std::abort();                                                            \
    }                                                                          \
  } while (false)
static bool close(double a, double b) {
  return std::fabs(a - b) <= 1e-12 * (1 + std::fabs(b));
}
using namespace superjupiter::sample_window;

int main() {
  (void)close;

  SampleWindow<3> window;
  window.push(1, UINT32_MAX - 9);
  window.push(NAN, 10);
  window.push(3, 20);
  window.push(5, 30);
  Summary summary = window.summary();
  CHECK(summary.storedCount == 3 && summary.finiteCount == 2 &&
        summary.invalidCount == 1);
  CHECK(summary.mean == 4 && summary.populationVariance == 1 &&
        summary.spanMs == 20);
  Sample output;
  output.value = 99;
  CHECK(!window.sampleAt(3, output) && output.value == 99);
  CHECK(!window.push(8, 29).accepted && window.summary().mean == 4);

  std::printf("PASS SuperJupiterSampleWindow review: %u checks\n", checks);
}
