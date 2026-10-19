# API and semantics

Include `SuperJupiterSampleWindow.h`. All public types are in
`superjupiter::sample_window`. The core requires C++11, normal non-finite floating
semantics and no external dependencies. Hardware validation remains pending.

## Configuration and capacity

`SampleWindow<Capacity>` holds one scalar stream. Capacity must be 1 through
65,535; unsuitable capacity is a compile-time error. The upper bound ensures a
sum of supported intervals fits uint64. Memory still limits practical capacities.

`Config::expectedIntervalMs` defaults to zero, disabling gap detection.
`gapToleranceMs` defaults to zero. A nonzero tolerance with zero expected interval
is invalid. Expected interval plus tolerance must not exceed `0x7fffffff`.
`validConfig(config)` checks this; `configured()` and `config()` expose the
constructed configuration. Invalid settings do not get silently corrected:
push returns `InputError::InvalidConfiguration` and history remains empty.

## `push(float value, uint32_t sampledAtMs)`

Submit an actual new observation and its capture time. No clock is read. First
capture can be at zero. Later captures must advance by 1 through `0x7fffffff`
milliseconds relative to the last accepted observation, using unsigned subtraction.
Duplicate, backward and unsupported-interval attempts return `InvalidTimestamp`
without changing history, last capture, eviction count or numeric state.

`PushResult::accepted` means an entry was stored, not that it was physically
accurate. Non-finite values are accepted and preserved with `finite == false`;
their error remains `None`. For rejected attempts `finite` remains false and
`error` explains the refusal. `overwroteOldest` reports exactly this insertion's
eviction. The oldest entry is replaced only once full.

`overwrittenSinceReset()` and the same Summary field count evictions, saturating
at `UINT32_MAX`. Saturation is not wraparound. They are lifetime-since-reset
counts, not the number of gaps, invalid values or missing physical samples.

## Raw entries

`sampleAt(index, output)` uses chronological indices from zero (oldest). It
returns false without changing output when out of bounds. `Sample` contains the
raw float, uint32 capture time and interval since the prior accepted entry.
`Sample::finite()` checks the value. No smoothing, calibration, range validation
or substitution occurs. NaN payload/signalling-bit preservation is not promised.

The first retained entry may have a nonzero `intervalMs` because its predecessor
was evicted. That incoming interval is outside the retained window and is excluded
from Summary span/gap calculations. Raw entries still expose it for inspection.

## Summary

`summary()` is a non-mutating snapshot in O(stored count). It is not a streaming
event log. `storedCount > 0` establishes that oldest/latest timestamps are present;
zero is a valid capture timestamp. `configured` reports configuration validity.

- Only finite values contribute to min/max/arithmetic mean/population variance.
  Finite outliers still contribute; there is no built-in range/quality policy.
- Empty/all-invalid windows have `hasStatistics == false`, `numericError == false`
  and zero-initialised statistic fields, which must not be treated as measurements.
- One finite entry has population variance zero. Otherwise the denominator is
  finiteCount, not finiteCount minus one. Mean and variance use Welford's algorithm.
- Calculations use target `double`; AVR may have float-width double. Non-finite
  intermediates/results or a negative accumulated squared-deviation value set
  `numericError` and clear `hasStatistics`. Statistics are zeroed on numeric error;
  counts and timing remain available. No hidden clipping or epsilon correction.
- Each entry has equal arithmetic weight. This is not time weighting or a
  guarantee that a particular duration is covered. Irregular spacing matters.
- uint64 `spanMs` sums the intervals BETWEEN retained entries. This supports
  retained histories covering multiple uint32 wraps if every consecutive accepted
  interval honours the half-range contract. It is not endpoint subtraction.
- Gap count/largest interval use those same intervals, including ones adjacent
  to invalid numeric entries. A gap is strictly greater than expected plus
  tolerance. No zero filling, interpolation or inferred missing count.
- Gap indicators disappear when their interval leaves the retained window.
  Overwritten-since-reset counts do not disappear until reset.

`reset()` clears logical entries, time history and counters, retaining settings.
It does not erase old sample bytes securely. Constructor initialisation is O(Capacity),
reset is O(1), push and raw lookup are O(1). No heap allocation occurs.

## Limits

Unsigned clocks alone cannot detect elapsed time of full wraps plus a small
interval. Caller chronology/interval guarantees are required. Reset after reboot,
clock replacement or epoch change. There is no locking or interrupt-safety claim.
Use externally serialised calls; copying/sharing large instances is the caller's
responsibility. No hard-real-time, acquisition-rate, accuracy or safety claim is made.
