// Refresh-delay experiment: how short can the grayscale refresh wait be?
//
// The Wing does not connect the panel's BUSY line, so Adafruit's driver waits a
// fixed default_refresh_delay (9000 ms for grayscale) and cannot tell when the
// panel is actually done. This sketch shortens that wait and draws alternating
// heavy/light cards so a truncated refresh shows up as fading or ghosting.
//
//   C = next delay in the list, redrawing the other card
//
// Reads /decks/rws/{59,70}.raw from the SD card.

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include "Adafruit_ThinkInk.h"

#define SD_CS     5
#define EPD_CS    9
#define EPD_DC    10
#define EPD_RESET -1
#define EPD_BUSY  -1
#define BUTTON_C  13

const uint8_t ROTATION = 3;
const uint16_t ART_W = 128;
const uint16_t ART_H = 224;
const size_t ART_BYTES = ((ART_W + 7) / 8) * ART_H;

// 9000 last, as a baseline to compare the short waits against.
const uint16_t DELAYS[] = {6000, 4000, 2000, 9000};
const int DELAY_COUNT = sizeof(DELAYS) / sizeof(DELAYS[0]);

// Ten of Swords (mostly black) and Seven of Pentacles (mostly white).
const int CARDS[] = {59, 70};

// default_refresh_delay is protected, so reach it through a subclass.
class TestDisplay : public ThinkInk_290_Grayscale4_FPC7519 {
 public:
  using ThinkInk_290_Grayscale4_FPC7519::ThinkInk_290_Grayscale4_FPC7519;
  void setRefreshDelay(uint16_t ms) { default_refresh_delay = ms; }
};

// Framebuffer in ESP32 RAM (SRAM CS = -1); see deckview.
TestDisplay display(EPD_DC, EPD_RESET, EPD_CS, -1, EPD_BUSY);

uint8_t art[ART_BYTES];
int step = 0;

bool readArt(int index) {
  char path[32];
  snprintf(path, sizeof(path), "/decks/rws/%02d.raw", index);
  File f = SD.open(path);
  if (!f) return false;
  size_t n = f.read(art, ART_BYTES);
  f.close();
  return n == ART_BYTES;
}

void render() {
  uint16_t ms = DELAYS[step % DELAY_COUNT];
  int card = CARDS[step % 2];
  display.setRefreshDelay(ms);
  bool artOk = readArt(card);

  display.clearBuffer();
  display.fillScreen(EPD_WHITE);
  if (artOk) display.drawBitmap(0, 0, art, ART_W, ART_H, EPD_BLACK);
  display.setTextColor(EPD_BLACK);
  display.setTextSize(2);
  char label[24];
  snprintf(label, sizeof(label), "delay %u ms", ms);
  display.setCursor(6, ART_H + 20);
  display.print(label);
  display.setTextSize(1);
  display.setCursor(6, ART_H + 48);
  display.printf("step %d, card %02d", step + 1, card);

  unsigned long t0 = millis();
  display.display();
  Serial.printf("step %d: refresh delay %u ms, card %02d, display() %lu ms%s\n",
                step + 1, ms, card, millis() - t0, artOk ? "" : "  [art missing]");
}

void setup() {
  Serial.begin(115200);
  unsigned long t0 = millis();
  while (!Serial && millis() - t0 < 5000) delay(10);
  delay(500);
  pinMode(BUTTON_C, INPUT_PULLUP);

  SPI.begin();
  display.begin(THINKINK_GRAYSCALE4);
  display.setColorBuffer(1, true);
  display.setBlackBuffer(0, true);
  display.setRotation(ROTATION);

  if (!SD.begin(SD_CS, SPI, 4000000)) {
    Serial.println("SD.begin failed");
    return;
  }
  Serial.println("press C for the next delay");
  render();
}

void loop() {
  static bool wasC = false;
  bool down = digitalRead(BUTTON_C) == LOW;
  if (down && !wasC) {
    step++;
    render();
  }
  wasC = down;
  delay(20);
}
