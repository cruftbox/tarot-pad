// Display test: draw one embedded card in the portrait layout from the brief.
// Art fills the top 128x224; name and keyword are rendered as text below.

#include <Arduino.h>
#include "Adafruit_ThinkInk.h"
#include "card.h"

// FeatherWing pins, as verified by hwcheck
#define SRAM_CS   6
#define EPD_CS    9
#define EPD_DC    10
#define EPD_RESET -1
#define EPD_BUSY  -1

// Portrait. 3 puts the card top at the red FPC tab end (1 was the USB-C end).
const uint8_t ROTATION = 3;

ThinkInk_290_Grayscale4_FPC7519 display(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);

void printCentered(const char* text, int16_t y) {
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, y, &x1, &y1, &w, &h);
  display.setCursor((display.width() - w) / 2, y);
  display.print(text);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // THINKINK_MONO left the previous image ghosted on white areas (2026-09-17,
  // unit 2). Grayscale mode with hwcheck's verified buffer settings instead.
  display.begin(THINKINK_GRAYSCALE4);
  display.setColorBuffer(1, true);
  display.setBlackBuffer(0, true);
  display.setRotation(ROTATION);
  display.clearBuffer();
  display.fillScreen(EPD_WHITE);

  display.drawBitmap(0, 0, CARD_BITS, CARD_W, CARD_H, EPD_BLACK);

  display.setTextColor(EPD_BLACK);
  display.setTextSize(1);
  printCentered(CARD_NAME, CARD_H + 20);
  printCentered(CARD_KEYWORD, CARD_H + 44);

  Serial.printf("canvas %dx%d, rotation %d\n", display.width(), display.height(), ROTATION);
  unsigned long t0 = millis();
  display.display();
  Serial.printf("display() returned after %lu ms\n", millis() - t0);
}

void loop() {}
