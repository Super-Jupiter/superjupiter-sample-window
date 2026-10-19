# Validation record

## Release 0.1.0

**Physical hardware validation: pending.** No actual ESP32, thermocouple or other
sensor has been run for this release. Software replay and successful board
compilation do not establish acquisition cadence, physical accuracy or timings.

The source and tests are original public work. Reused MIT build/release scaffolding
comes from SuperJupiter SensorQuality 0.2.0, not proprietary application projects.

## Software validation gates

Consult the actual [CI](https://github.com/Super-Jupiter/superjupiter-sample-window/actions)
for the result of these checks on the release commit:

- GCC/Clang strict C++11 host tests with address/undefined-behaviour sanitizers.
- Independent Decimal batch-reference fixtures for two capacities and six input
  sequences: exact values, mixed signs, offset/small variation and decimal inputs.
- Empty/reset/bounds, capacity one/two, chronological eviction, preserved NaN/Inf
  and signed zero, invalid configuration, duplicate/backward/half-range clocks,
  rollover and a seven-billion-ms retained span over multiple clock wraps.
- Exact gap boundary and removal, overflow handling, saturation helper boundary,
  100,000-observation stream, independent channels and no allocations during core use.
- Namespace isolation, incompatible fast-math rejection and invalid capacity checks.
- Source/exported-package CMake subproject and relocated installed consumers.
- Export validation, actual ZIP installation and clean PlatformIO package consumer.
- Both bundled examples and optional SensorQuality integration compiled for
  `esp32:esp32@3.3.12` ESP32/S3/C3 and `arduino:avr@1.8.8` Uno.

Most assertions in the stream test are repeated checks, not distinct cases.
Host `sizeof` results apply only to that compiler/ABI. Numeric overflow behaviour
on float-width double is specified; board compilation does not execute it.
Release/archive checksums and registry processing are verified separately.

## First real hardware session

1. Record actual board/module, compiler/core versions and release commit.
2. Upload MinimalWindow and SyntheticTemperatureWindow, capture serial at115200.
3. Compare actual raw/invalid/gap/overwrite output to documented synthetic inputs.
4. Integrate an established sensor driver separately; verify fresh measurements
   and actual conversion cadence rather than loop frequency.
5. Record memory, summary/push timing, disconnect/recovery and measured output.
6. State exactly what was executed and any limitations. Do not imply one board
   test validates every board or demonstrates sensor accuracy.
