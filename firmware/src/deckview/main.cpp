// Deck demo: show cards from the decks on the SD card.
//   C = switch to the next deck (same card number, for comparison)
//   A = random card in the current deck
//
// SD layout, as written by tools/convert_cards.py:
//   /decks/<deck>/NN.raw     128x224, 1 = black, MSB first, byte-padded rows
//   /decks/<deck>/cards.csv  index,name,keyword

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include "../common/cards.h"

// FeatherWing pins, as verified by hwcheck
#define SD_CS     5
#define EPD_CS    9
#define EPD_DC    10
#define EPD_RESET -1
#define EPD_BUSY  -1

#define BUTTON_A  11
#define BUTTON_C  13

const uint8_t ROTATION = 3;  // card top at the red FPC tab end
const int MAX_DECKS = 16;
// Deck to show at boot, if present; otherwise the first one found.
const char* START_DECK = "rws";

// The Wing leaves the panel's BUSY line unconnected, so the driver waits a fixed
// default_refresh_delay (9000 ms for grayscale) instead of watching the panel.
// 2000 ms drew cleanly on unit 2 with heavy-black and mostly-white cards
// (refreshtest, 2026-09-17); it is protected, so reach it through a subclass.
class CardDisplay : public ThinkInk_290_Grayscale4_FPC7519 {
 public:
  using ThinkInk_290_Grayscale4_FPC7519::ThinkInk_290_Grayscale4_FPC7519;
  void setRefreshDelay(uint16_t ms) { default_refresh_delay = ms; }
};

const uint16_t REFRESH_DELAY_MS = 2000;

// Framebuffer in ESP32 RAM (SRAM CS = -1), not the Wing's SPI SRAM: with SRAM,
// every pixel costs four SPI transactions and a full-screen fill took ~3.9 s.
CardDisplay display(EPD_DC, EPD_RESET, EPD_CS, -1, EPD_BUSY);

String decks[MAX_DECKS];
int deckCount = 0;
int deckIdx = 0;
int cardIdx = 0;
uint8_t art[ART_BYTES];


void showMessage(const char* msg) {
  display.clearBuffer();
  display.fillScreen(EPD_WHITE);
  display.setTextColor(EPD_BLACK);
  display.setTextSize(1);
  printWrapped(display, msg, 120);
  display.display();
}

void render() {
  const String& deck = decks[deckIdx];
  unsigned long tStart = millis();
  Card card;
  int count = readCards(deck, cardIdx, card);
  if (count > 0 && cardIdx >= count) {
    cardIdx = cardIdx % count;
    readCards(deck, cardIdx, card);
  }
  bool artOk = readArt(deck, cardIdx, art);
  Serial.printf("deck %s (%d cards), card %02d: %s / %s%s%s\n", deck.c_str(), count, cardIdx,
                card.name.c_str(), card.keyword.c_str(), card.titled ? "  [titled]" : "",
                artOk ? "" : "  [art missing]");

  unsigned long tRead = millis();
  drawCard(display, art, artOk, card);
  unsigned long t0 = millis();
  Serial.printf("  timing: SD read %lu ms, draw %lu ms\n", tRead - tStart, t0 - tRead);
  display.display();
  Serial.printf("  display() %lu ms\n", millis() - t0);
}

bool pressed(uint8_t pin, bool& was) {
  bool down = digitalRead(pin) == LOW;
  bool edge = down && !was;
  was = down;
  return edge;
}

void setup() {
  Serial.begin(115200);
  unsigned long t0 = millis();
  while (!Serial && millis() - t0 < 5000) delay(10);
  delay(500);
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);

  SPI.begin();
  // THINKINK_MONO left the previous image ghosted; grayscale init clears fully.
  display.begin(THINKINK_GRAYSCALE4);
  display.setRefreshDelay(REFRESH_DELAY_MS);
  display.setColorBuffer(1, true);
  display.setBlackBuffer(0, true);
  display.setRotation(ROTATION);

  if (!SD.begin(SD_CS, SPI, 4000000)) {
    Serial.println("SD.begin failed");
    showMessage("SD card not found");
    return;
  }
  deckCount = findDecks(decks, MAX_DECKS);
  Serial.printf("%d decks:", deckCount);
  for (int i = 0; i < deckCount; i++) Serial.printf(" %s", decks[i].c_str());
  Serial.println();
  if (deckCount == 0) {
    showMessage("No decks in /decks");
    return;
  }
  for (int i = 0; i < deckCount; i++)
    if (decks[i] == START_DECK) deckIdx = i;
  cardIdx = esp_random() % 78;
  render();
}

void loop() {
  static bool wasA = false, wasC = false;
  if (deckCount == 0) return;
  if (pressed(BUTTON_C, wasC)) {
    deckIdx = (deckIdx + 1) % deckCount;
    render();
  } else if (pressed(BUTTON_A, wasA)) {
    Card unused;
    int count = readCards(decks[deckIdx], -1, unused);
    cardIdx = esp_random() % (count > 0 ? count : 78);
    render();
  }
  // Presses during the ~10 s refresh are not queued.
  delay(20);
}
