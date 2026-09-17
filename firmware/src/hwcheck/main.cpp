// Hardware verification for ESP32-S3 Feather (5477) + 2.9" Grayscale eInk
// FeatherWing (4777). Answers the open questions in tarot-display-brief.md.
//
// Pin numbers below are the S3 Feather's silkscreen D-numbers, which equal
// GPIO numbers on this board. The Adafruit FeatherWingGray4 example uses
// original-ESP32 Feather pins under `#if defined(ESP32)`; those are wrong here.

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <Adafruit_MAX1704X.h>
#include "Adafruit_ThinkInk.h"

// FeatherWing pins
#define SD_CS     5
#define SRAM_CS   6
#define EPD_CS    9
#define EPD_DC    10
#define EPD_RESET -1  // tied to Feather RESET
#define EPD_BUSY  -1  // not broken out on the Wing

// Free GPIOs watched for button presses. A/B/C are expected on D11/D12/D13,
// but this is watched rather than assumed.
const uint8_t WATCH_PINS[] = {8, 11, 12, 13, 14, 15, 16, 17, 18};
const size_t WATCH_COUNT = sizeof(WATCH_PINS);

ThinkInk_290_Grayscale4_FPC7519 display(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);
Adafruit_MAX17048 maxlipo;

String report;
int lastLevel[sizeof(WATCH_PINS)];

void log(const String& line) {
  Serial.println(line);
  report += line + "\n";
}

bool testSram() {
  // The Wing's SPI SRAM is the one part that proves the Wing is attached
  // regardless of SD card or panel state. A missing chip reads back 0xFF or
  // noise, so write distinct values and compare.
  Adafruit_MCPSRAM sram(SRAM_CS, &SPI);
  sram.begin();
  sram.write8(0, K640_SEQUENTIAL_MODE, MCPSRAM_WRSR);
  const uint16_t addrs[] = {0x0000, 0x1234, 0x7FF0};
  const uint8_t vals[] = {0xA5, 0x5A, 0x3C};
  bool ok = true;
  for (int i = 0; i < 3; i++) sram.write8(addrs[i], vals[i]);
  for (int i = 0; i < 3; i++) {
    uint8_t got = sram.read8(addrs[i]);
    log("  SRAM[0x" + String(addrs[i], HEX) + "] wrote 0x" + String(vals[i], HEX) +
        " read 0x" + String(got, HEX));
    if (got != vals[i]) ok = false;
  }
  return ok;
}

void listDir(File dir, int depth) {
  while (File entry = dir.openNextFile()) {
    String indent = "";
    for (int i = 0; i < depth + 1; i++) indent += "  ";
    if (entry.isDirectory()) {
      log(indent + entry.name() + "/");
      if (depth < 2) listDir(entry, depth + 1);
    } else {
      log(indent + entry.name() + "  " + String(entry.size()) + " B");
    }
    entry.close();
  }
}

bool testSd() {
  if (!SD.begin(SD_CS, SPI, 4000000)) {
    log("  SD.begin failed (no card, bad format, or no Wing)");
    return false;
  }
  const char* types[] = {"NONE", "MMC", "SD", "SDHC", "UNKNOWN"};
  uint8_t t = SD.cardType();
  log("  card type " + String(t <= 4 ? types[t] : "?") + ", size " +
      String((uint32_t)(SD.cardSize() / (1024 * 1024))) + " MB");
  File root = SD.open("/");
  listDir(root, 0);
  root.close();
  SD.end();
  return true;
}

void testBattery() {
  pinMode(PIN_I2C_POWER, OUTPUT);
  digitalWrite(PIN_I2C_POWER, HIGH);
  delay(10);
  Wire.begin();
  String found = "";
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) found += "0x" + String(a, HEX) + " ";
  }
  log("  I2C devices: " + (found.length() ? found : String("none")));
  if (!maxlipo.begin()) {
    log("  MAX17048 not found");
    return;
  }
  log("  MAX17048 chip id 0x" + String(maxlipo.getChipID(), HEX));
  log("  battery " + String(maxlipo.cellVoltage(), 3) + " V, " +
      String(maxlipo.cellPercent(), 1) + " %");
}

void drawTestPattern(bool sramOk, bool sdOk) {
  display.begin(THINKINK_GRAYSCALE4);
  // Same polarity and rotation settings as Adafruit's FeatherWingGray4 example
  // for this panel revision.
  display.setColorBuffer(1, true);
  display.setBlackBuffer(0, true);
  display.setRotation(0);

  display.clearBuffer();
  display.fillScreen(EPD_WHITE);
  display.drawRect(0, 0, display.width(), display.height(), EPD_BLACK);
  display.setTextColor(EPD_BLACK);
  display.setTextSize(2);
  display.setCursor(6, 6);
  display.print("tarot-pad hwcheck");
  display.setTextSize(1);
  display.setCursor(6, 30);
  display.print(String("SRAM ") + (sramOk ? "OK" : "FAIL") + "   SD " + (sdOk ? "OK" : "FAIL"));
  display.setTextColor(EPD_DARK);
  display.setCursor(6, 44);
  display.print("dark gray text");

  int16_t w = display.width();
  int16_t seg = w / 4, top = display.height() - 40, h = 34;
  uint16_t ramp[4] = {EPD_BLACK, EPD_DARK, EPD_LIGHT, EPD_WHITE};
  for (int i = 0; i < 4; i++) {
    display.fillRect(i * seg, top, (i < 3 ? seg : w - i * seg), h, ramp[i]);
  }
  display.drawRect(0, top, w, h, EPD_BLACK);
  unsigned long t0 = millis();
  display.display();
  log("  display() returned after " + String(millis() - t0) + " ms");
}

void setup() {
  Serial.begin(115200);
  unsigned long t0 = millis();
  while (!Serial && millis() - t0 < 5000) delay(10);
  delay(500);

  log("=== tarot-pad hwcheck ===");

  SPI.begin();

  log("[SRAM]");
  bool sramOk = testSram();
  log(String("  RESULT: ") + (sramOk ? "PASS - FeatherWing SRAM responds" : "FAIL"));

  log("[SD]");
  bool sdOk = testSd();
  log(String("  RESULT: ") + (sdOk ? "PASS" : "FAIL"));

  log("[Battery]");
  testBattery();

  log("[Buttons] internal pull-ups enabled; initial levels:");
  for (size_t i = 0; i < WATCH_COUNT; i++) {
    uint8_t p = WATCH_PINS[i];
    pinMode(p, INPUT_PULLUP);
  }
  delay(20);
  for (size_t i = 0; i < WATCH_COUNT; i++) {
    uint8_t p = WATCH_PINS[i];
    lastLevel[i] = digitalRead(p);
    log("  GPIO" + String(p) + " = " + String(lastLevel[i]) +
        (p <= 21 ? "  (RTC-capable)" : "  (NOT RTC-capable)"));
  }

  log("[Display] drawing test pattern");
  drawTestPattern(sramOk, sdOk);

  log("=== setup done; press buttons A, B, C to map pins ===");
}

void loop() {
  static unsigned long lastReport = 0;
  for (size_t i = 0; i < WATCH_COUNT; i++) {
    int level = digitalRead(WATCH_PINS[i]);
    if (level != lastLevel[i]) {
      lastLevel[i] = level;
      Serial.println("BUTTON GPIO" + String(WATCH_PINS[i]) + " -> " +
                     (level ? "HIGH (released)" : "LOW (pressed)"));
    }
  }
  // Repeat the setup report so it can be read after connecting late.
  if (millis() - lastReport > 15000) {
    lastReport = millis();
    Serial.print("\n--- report ---\n" + report + "--- end report ---\n");
  }
  delay(10);
}
