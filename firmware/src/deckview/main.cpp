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
#include "Adafruit_ThinkInk.h"

// FeatherWing pins, as verified by hwcheck
#define SD_CS     5
#define SRAM_CS   6
#define EPD_CS    9
#define EPD_DC    10
#define EPD_RESET -1
#define EPD_BUSY  -1

#define BUTTON_A  11
#define BUTTON_C  13

const uint8_t ROTATION = 3;  // card top at the red FPC tab end
const uint16_t ART_W = 128;
const uint16_t ART_H = 224;
const size_t ART_BYTES = ((ART_W + 7) / 8) * ART_H;
const char* DECKS_DIR = "/decks";
const int MAX_DECKS = 16;

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

struct Card {
  String name;
  String keyword;
};

void findDecks() {
  File root = SD.open(DECKS_DIR);
  Serial.printf("open %s: %s, isDirectory %d\n", DECKS_DIR, root ? "ok" : "FAILED", root ? root.isDirectory() : 0);
  if (!root || !root.isDirectory()) return;
  while (File entry = root.openNextFile()) {
    Serial.printf("  entry '%s' dir=%d\n", entry.name(), entry.isDirectory());
    if (entry.isDirectory() && deckCount < MAX_DECKS) decks[deckCount++] = entry.name();
    entry.close();
  }
  root.close();
  // Directory order on FAT is creation order; sort so the cycle is predictable.
  for (int i = 1; i < deckCount; i++)
    for (int j = i; j > 0 && decks[j] < decks[j - 1]; j--) std::swap(decks[j], decks[j - 1]);
}

String field(const String& s, int& pos) {
  // Minimal CSV field: handles the double-quoted fields Python's csv writer
  // emits when a value contains a comma or quote.
  String out;
  if (pos < (int)s.length() && s[pos] == '"') {
    pos++;
    while (pos < (int)s.length()) {
      if (s[pos] == '"') {
        if (pos + 1 < (int)s.length() && s[pos + 1] == '"') { out += '"'; pos += 2; continue; }
        pos++;
        break;
      }
      out += s[pos++];
    }
  } else {
    while (pos < (int)s.length() && s[pos] != ',') out += s[pos++];
  }
  if (pos < (int)s.length() && s[pos] == ',') pos++;
  return out;
}

// Returns the number of cards in the deck, filling `card` if `index` is found.
int readCards(const String& deck, int index, Card& card) {
  File f = SD.open(String(DECKS_DIR) + "/" + deck + "/cards.csv");
  if (!f) return 0;
  f.readStringUntil('\n');  // header
  int count = 0;
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;
    count++;
    int pos = 0;
    if (field(line, pos).toInt() == index) {
      card.name = field(line, pos);
      card.keyword = field(line, pos);
    }
  }
  f.close();
  return count;
}

bool readArt(const String& deck, int index) {
  char path[64];
  snprintf(path, sizeof(path), "%s/%s/%02d.raw", DECKS_DIR, deck.c_str(), index);
  File f = SD.open(path);
  if (!f) return false;
  size_t n = f.read(art, ART_BYTES);
  f.close();
  return n == ART_BYTES;
}

// Print text centered, wrapping at spaces to fit the width. Returns the next y.
int16_t printWrapped(const String& text, int16_t y) {
  const int16_t lineH = 12;
  String rest = text;
  while (rest.length()) {
    String line = rest;
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(line, 0, 0, &x1, &y1, &w, &h);
    while (w > display.width() && line.lastIndexOf(' ') > 0) {
      line = line.substring(0, line.lastIndexOf(' '));
      display.getTextBounds(line, 0, 0, &x1, &y1, &w, &h);
    }
    display.setCursor((display.width() - w) / 2, y);
    display.print(line);
    rest = rest.substring(line.length());
    rest.trim();
    y += lineH;
  }
  return y;
}

void showMessage(const char* msg) {
  display.clearBuffer();
  display.fillScreen(EPD_WHITE);
  display.setTextColor(EPD_BLACK);
  printWrapped(msg, 120);
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
  bool artOk = readArt(deck, cardIdx);
  Serial.printf("deck %s (%d cards), card %02d: %s / %s%s\n", deck.c_str(), count, cardIdx,
                card.name.c_str(), card.keyword.c_str(), artOk ? "" : "  [art missing]");

  unsigned long tRead = millis();
  display.clearBuffer();
  display.fillScreen(EPD_WHITE);
  unsigned long tClear = millis();
  if (artOk) display.drawBitmap(0, 0, art, ART_W, ART_H, EPD_BLACK);
  display.setTextColor(EPD_BLACK);
  display.setTextSize(1);
  int16_t y = printWrapped(card.name, ART_H + 14);
  printWrapped(card.keyword, y + 10);
  unsigned long t0 = millis();
  Serial.printf("  timing: SD read %lu ms, clear+fill %lu ms, bitmap+text %lu ms\n",
                tRead - tStart, tClear - tRead, t0 - tClear);
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
  findDecks();
  Serial.printf("%d decks:", deckCount);
  for (int i = 0; i < deckCount; i++) Serial.printf(" %s", decks[i].c_str());
  Serial.println();
  if (deckCount == 0) {
    showMessage("No decks in /decks");
    return;
  }
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
