// SPDX-License-Identifier: MIT
// Synthetic values only. No sensor or physical validation is implied.
#include <SuperJupiterSampleWindow.h>

superjupiter::sample_window::SampleWindow<8> samples;
uint32_t previousAt = 0;

void setup() { Serial.begin(115200); }

void loop() {
  const uint32_t now = millis();
  if (now - previousAt < 100) return;
  previousAt = now;
  const float syntheticValue = 20.0f;
  const superjupiter::sample_window::PushResult pushed = samples.push(syntheticValue, now);
  const superjupiter::sample_window::Summary result = samples.summary();
  Serial.print("count="); Serial.print(static_cast<unsigned>(result.storedCount));
  Serial.print(" overwritten="); Serial.print(result.overwrittenSinceReset);
  Serial.print(" new_overwrite="); Serial.print(pushed.overwroteOldest);
  if (result.hasStatistics) {
    Serial.print(" mean="); Serial.print(result.mean, 3);
  }
  Serial.println();
}
