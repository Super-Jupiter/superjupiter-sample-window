# SuperJupiter SampleWindow

[![CI](https://github.com/Super-Jupiter/superjupiter-sample-window/actions/workflows/ci.yml/badge.svg)](https://github.com/Super-Jupiter/superjupiter-sample-window/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://github.com/Super-Jupiter/superjupiter-sample-window/blob/main/LICENSE)

**Keep timestamped raw samples and make the limits of their statistics visible.**

A small, allocation-free C++11 last-N-observations window for Arduino, ESP32 and
host applications. Preserve raw values, inspect them in chronological order and
report finite-only statistics, invalid values, observed gaps and overwrites.

**Early preview, 0.1.0. Physical hardware validation is pending.** Software tests
and board compilation are separate from real device measurements. This is not a
sensor driver, calibration system, physical fault detector or safety certification.

```text
raw value + capture timestamp -> fixed-capacity sample window -> summary
                                      |
                                      +-> original raw entries, including NaN/Inf
```

## Installation

### Arduino

Search Library Manager for **SuperJupiterSampleWindow**, author **SuperJupiter**.
Registration/index processing can lag a release; do not assume a cached index
contains it. For an exact version after indexing:

```sh
arduino-cli lib update-index
arduino-cli lib install "SuperJupiterSampleWindow@0.1.0"
```

The [release ZIP](https://github.com/Super-Jupiter/superjupiter-sample-window/releases/tag/v0.1.0)
also works with **Sketch > Include Library > Add .ZIP Library**. Open
**File > Examples > SuperJupiterSampleWindow > MinimalWindow** and select
**115200 baud**. No sensor is needed for either bundled example.

### PlatformIO

Use the [registry package](https://registry.platformio.org/libraries/superjupiter/SuperJupiterSampleWindow)
with an exact pin after publication:

```ini
lib_deps = superjupiter/SuperJupiterSampleWindow@0.1.0
```

`@^0.1.0` permits compatible updates; it is not an exact pin. A source alternative
is `https://github.com/Super-Jupiter/superjupiter-sample-window.git#v0.1.0`.

## Quick start

```cpp
#include <SuperJupiterSampleWindow.h>

namespace window_api = superjupiter::sample_window;

window_api::Config configuration() {
  window_api::Config c;
  c.expectedIntervalMs = 100;  // Example 10 Hz observation schedule.
  c.gapToleranceMs = 30;
  return c;
}

window_api::SampleWindow<16> samples(configuration());

void onFreshReading(float value, uint32_t sampledAtMs) {
  const window_api::PushResult pushed = samples.push(value, sampledAtMs);
  if (!pushed.accepted) {
    // Inspect pushed.error; a rejected attempt did not change the window.
    return;
  }
  const window_api::Summary s = samples.summary();
  if (s.hasStatistics) {
    // s.mean and s.populationVariance describe finite entries only.
    // Check s.invalidCount, s.hasGap and s.overwrittenSinceReset separately.
  }
  window_api::Sample oldest;
  if (samples.sampleAt(0, oldest)) {
    // oldest.value is the original value, not a filtered replacement.
  }
}
```

NaN and infinity are accepted as raw entries but excluded from arithmetic. An
empty or entirely invalid window has `hasStatistics == false`. Non-finite
arithmetic sets `numericError`, rather than returning a plausible-looking mean.

## What the summary means

| Field | Meaning |
| --- | --- |
| `storedCount`, `full` | Current entries and whether capacity has been reached. |
| `finiteCount`, `invalidCount` | Arithmetic contributors and non-finite entries. |
| `minimum`, `maximum`, `mean` | Finite-only statistics, valid when `hasStatistics`. |
| `populationVariance` | Sum of squared deviations divided by finite count, not sample variance. |
| `oldestAtMs`, `latestAtMs` | Original uint32 capture timestamps; use `storedCount` to establish presence. |
| `spanMs` | uint64 sum of intervals inside the retained window, not uptime. |
| `largestIntervalMs`, `gapCount`, `hasGap` | Observed intervals inside this window, including invalid entries. |
| `overwrittenSinceReset` | Saturating lifetime count of ring evictions, not missing sensor samples. |

This is a **sample-weighted**, last-N window. Irregularly sampled values receive
equal weight. It is not a fixed-duration or time-weighted average. A 600-entry
window at a genuine 10 Hz spans 59.9 seconds between first and last capture.
The library does not infer sampling rate or count missing readings from a gap.

`push()` is O(1), `summary()` is O(stored count), and raw lookup is O(1).
Capacity is fixed at compile time, from 1 to 65,535. Each stored entry also keeps
the preceding accepted interval to support retained windows spanning clock
rollovers. Measure memory on the actual target; capacity 600 is unsuitable for
an Uno's RAM. The bundled Arduino examples use 8 and 16 entries.

## Clocks, numeric limits and concurrency

- Submit **fresh observations**, not a cached reading with a new timestamp.
- Consecutive timestamps must advance by 1 through `0x7fffffff` ms. A normal
  `millis()` rollover is valid. Reset after reboot or changing clocks.
- Timestamps alone cannot prove that several complete wraps did not pass between
  calls. The caller must honour the consecutive-interval limit.
- Raw NaN/Inf are preserved as values, but NaN payload-bit preservation is not an
  API guarantee. Float input precision and target `double` width still apply.
- Do not enable fast-math/finite-math-only assumptions; known GCC/Clang modes
  are rejected. Thread/ISR synchronisation belongs to the application.
- Reset clears logical history and counters; it is not a secure memory erase.

See [API and exact semantics](https://github.com/Super-Jupiter/superjupiter-sample-window/blob/main/docs/API.md).

## SensorQuality integration

SampleWindow does not depend on [SuperJupiter SensorQuality](https://github.com/Super-Jupiter/superjupiter-sensor-quality).
The optional sketch in `extras/WithSensorQuality` uses version 0.2.0 separately to
show both namespaces together. Quality flags and window statistics answer
different questions: finite, out-of-range values still contribute to SampleWindow
statistics unless your application explicitly chooses another selection policy.

There is no proprietary domain model, smoothing, derivative estimator, network
service or durable storage in this component. For a basic queue or rolling mean,
also consider established [CircularBuffer](https://github.com/rlogiacco/CircularBuffer)
or [RunningAverage](https://github.com/RobTillaart/RunningAverage). Timestamped
statistics also exist in [RunningStatistics](https://github.com/soosp/RunningStatistics).
Our focus is the documented raw-entry, invalid-value, timing and overwrite contract;
we do not claim rolling statistics or circular buffers are novel.

## Tests and CMake

```sh
python3 tools/generate_reference.py --check
python3 tools/check_release.py
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
python3 tools/verify_consumers.py
```

Consumers use `add_subdirectory(...)` and `SuperJupiter::SampleWindow`, or
`find_package(SuperJupiterSampleWindow 0.1 CONFIG REQUIRED)` after installation.
Tests, examples and install rules default OFF for subproject builds. See
[conventions](https://github.com/Super-Jupiter/superjupiter-sample-window/blob/main/docs/CONVENTIONS.md).

CI checks GCC/Clang sanitizers, independent Decimal reference fixtures, package
and relocated CMake consumers, namespace isolation, a PlatformIO package consumer,
and ESP32/S3/C3/Uno compilation of synthetic examples, installed ZIPs and the
optional SensorQuality integration. Consult the actual
[CI results](https://github.com/Super-Jupiter/superjupiter-sample-window/actions)
and [validation record](https://github.com/Super-Jupiter/superjupiter-sample-window/blob/main/docs/VALIDATION.md).

## Support and licence

Original standalone code by [SuperJupiter](https://www.superjupiter.net), under
the [MIT licence](https://github.com/Super-Jupiter/superjupiter-sample-window/blob/main/LICENSE).
No SuperJupiter service is required. SuperJupiter builds custom software, connected
devices and operational platforms. Contact **info@superjupiter.net**.

Read [CONTRIBUTING.md](https://github.com/Super-Jupiter/superjupiter-sample-window/blob/main/CONTRIBUTING.md)
and [SECURITY.md](https://github.com/Super-Jupiter/superjupiter-sample-window/blob/main/SECURITY.md).
No guaranteed support response time, hardware validation or maintenance service
level is claimed.
