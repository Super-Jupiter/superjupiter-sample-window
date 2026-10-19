// SPDX-License-Identifier: MIT
// Entirely synthetic. No sensor is read and hardware validation is pending.
#include <SuperJupiterSampleWindow.h>
#include <math.h>

superjupiter::sample_window::Config settings() {
  superjupiter::sample_window::Config c;
  c.expectedIntervalMs = 100;
  c.gapToleranceMs = 30;
  return c;
}

superjupiter::sample_window::SampleWindow<16> samples(settings());
uint32_t previousAt = 0;
uint32_t step = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("SuperJupiter SampleWindow: SYNTHETIC data; hardware validation pending");
}

void loop() {
  const uint32_t now = millis();
  if (now - previousAt < 100) return;
  previousAt = now;
  const uint32_t phase = step++ % 40;
  if (phase >= 20 && phase < 25) return;  // No invented samples during the gap.
  float value = 20.0f + static_cast<float>(phase % 5) * 0.25f;
  if (phase == 10) value = NAN;
  const superjupiter::sample_window::PushResult pushed = samples.push(value, now);
  const superjupiter::sample_window::Summary result = samples.summary();
  Serial.print("raw="); Serial.print(value);
  Serial.print(" finite="); Serial.print(pushed.finite);
  Serial.print(" invalid="); Serial.print(static_cast<unsigned>(result.invalidCount));
  Serial.print(" gaps="); Serial.print(result.gapCount);
  Serial.print(" overwritten="); Serial.print(result.overwrittenSinceReset);
  if (result.hasStatistics) {
    Serial.print(" mean="); Serial.print(result.mean, 3);
    Serial.print(" variance="); Serial.print(result.populationVariance, 3);
  }
  Serial.println();
}
