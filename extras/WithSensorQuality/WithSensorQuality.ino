// SPDX-License-Identifier: MIT
// OPTIONAL integration: install SuperJupiterSensorQuality 0.2.0 separately.
// This sketch uses synthetic inputs, not a real sensor.
#include <SuperJupiterSampleWindow.h>
#include <SuperJupiterSensorQuality.h>
#include <math.h>

namespace window_api = superjupiter::sample_window;
namespace quality_api = superjupiter::sensor_quality;
quality_api::Config qualitySettings() {
  quality_api::Config c;
  c.rangeEnabled = true;
  c.minimum = -40;
  c.maximum = 125;
  c.staleAfterMs = 500;
  return c;
}

window_api::SampleWindow<16> samples;
quality_api::SensorQuality quality(qualitySettings());
uint32_t previousAt = 0;
uint32_t step = 0;

void setup() { Serial.begin(115200); }
void loop() {
  const uint32_t now = millis();
  if (now - previousAt < 100) return;
  previousAt = now;
  const float value = (++step % 12 == 0) ? NAN : 20.0f;
  const quality_api::Result checked = quality.observe(value, now);
  samples.push(value, now);  // Preserve all raw entries, including flagged ones.
  const window_api::Summary result = samples.summary();
  Serial.print("quality_flags="); Serial.print(checked.flags);
  Serial.print(" finite_values="); Serial.print(static_cast<unsigned>(result.finiteCount));
  // Finite out-of-range readings remain part of SampleWindow statistics.
  // Apply any additional quality-selection policy explicitly in your application.
  if (result.hasStatistics) { Serial.print(" mean="); Serial.print(result.mean); }
  Serial.println();
}
