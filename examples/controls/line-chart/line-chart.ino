/**
 * @file line-chart.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `LineChart`.
 *
 * Two charts: a rolling trace that scrolls in from the right
 * (setVisiblePoints()) with a fixed range and a dashed threshold, and an
 * auto-scaled chart with two series and a footer.
 *
 * Builds and runs unchanged on a real ESP32 board (see kitchen-sink.ino for
 * the board-selection pattern) and, via LCDBoardDesktopSDL, in a desktop
 * SDL2 window for mouse-driven testing without touch hardware.
 */
#include <math.h>

#include <TinyMaterialDesign.h>

App<RGB565> app(DefaultBoard);

// --- LineChart (the control under test) --------------------------------------
LineChart<RGB565> rollingChart(Bounds(8, 8, app.width() - 16, 140), "Rolling trace");
LineChart<RGB565> autoChart(Bounds(8, 156, app.width() - 16, 156), "Auto scale");
int rollingSeries;
int sineSeries, cosineSeries;
uint32_t lastUpdate = 0;
float phase = 0.0f;

void setup() {
  Serial.begin(115200);
  app.begin();

  rollingSeries = rollingChart.addSeries(colorFromHex<RGB565>(0x06B6D4), 1);
  rollingChart.setYRange(-1.0f, 1.0f);
  rollingChart.setVisiblePoints(100);
  rollingChart.addReferenceLine(0.6f, colorFromHex<RGB565>(0xF59E0B));

  sineSeries = autoChart.addSeries(colorFromHex<RGB565>(0x10B981));
  cosineSeries = autoChart.addSeries(colorFromHex<RGB565>(0xA78BFA));

  app.screen().addWidget(rollingChart);
  app.screen().addWidget(autoChart);
}

void loop() {
  if (millis() - lastUpdate > 50) {
    lastUpdate = millis();
    phase += 0.15f;
    // keep only the newest 100 points of the rolling trace
    rollingChart.addValue(rollingSeries, sinf(phase) * cosf(phase * 0.13f), 100);

    std::vector<float> s, c;
    for (int i = 0; i < 60; ++i) {
      s.push_back(30.0f * sinf(phase * 0.2f + i * 0.1f) + 50.0f);
      c.push_back(i == 30 ? NAN : 20.0f * cosf(phase * 0.2f + i * 0.1f) + 40.0f);  // NaN = gap
    }
    autoChart.setValues(sineSeries, s);
    autoChart.setValues(cosineSeries, c);
    autoChart.setFooter("NaN values break the line");

    // the data changed outside of a gesture - see Screen::isDirty()
    app.screen().invalidate();
  }
  app.update();
}
