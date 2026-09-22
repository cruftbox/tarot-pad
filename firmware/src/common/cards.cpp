#include "cards.h"

#include <SD.h>

static const char* DECKS_DIR = "/decks";

int findDecks(String* decks, int maxDecks) {
  int count = 0;
  File root = SD.open(DECKS_DIR);
  if (!root || !root.isDirectory()) return 0;
  while (File entry = root.openNextFile()) {
    if (entry.isDirectory() && count < maxDecks) decks[count++] = entry.name();
    entry.close();
  }
  root.close();
  // Directory order on FAT is creation order; sort so the cycle is predictable.
  for (int i = 1; i < count; i++)
    for (int j = i; j > 0 && decks[j] < decks[j - 1]; j--) std::swap(decks[j], decks[j - 1]);
  return count;
}

// Minimal CSV field: handles the double-quoted fields Python's csv writer
// emits when a value contains a comma or quote.
static String field(const String& s, int& pos) {
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
      card.titled = field(line, pos).toInt() == 1;
    }
  }
  f.close();
  return count;
}

bool readArt(const String& deck, int index, uint8_t* buf) {
  char path[64];
  snprintf(path, sizeof(path), "%s/%s/%02d.raw", DECKS_DIR, deck.c_str(), index);
  File f = SD.open(path);
  if (!f) return false;
  size_t n = f.read(buf, ART_BYTES);
  f.close();
  return n == ART_BYTES;
}

static const int16_t LINE_H = 12;

// Walks `text` the way printWrapped draws it, counting the lines it needs.
static int wrappedLines(Adafruit_EPD& display, const String& text) {
  int lines = 0;
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
    rest = rest.substring(line.length());
    rest.trim();
    lines++;
  }
  return lines;
}

int16_t printWrapped(Adafruit_EPD& display, const String& text, int16_t y) {
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
    y += LINE_H;
  }
  return y;
}

void drawCard(Adafruit_EPD& display, const uint8_t* art, bool artOk, const Card& card) {
  display.clearBuffer();
  display.fillScreen(EPD_WHITE);
  if (artOk) display.drawBitmap(0, 0, art, ART_W, ART_H, EPD_BLACK);
  display.setTextColor(EPD_BLACK);
  display.setTextSize(1);
  if (card.titled) {
    // Only the keyword to place, so centre it in the strip below the art.
    // The last line needs no leading, hence LINE_H - 4 for the glyph height.
    int16_t h = wrappedLines(display, card.keyword) * LINE_H - 4;
    printWrapped(display, card.keyword, ART_H + (display.height() - ART_H - h) / 2);
  } else {
    int16_t y = printWrapped(display, card.name, ART_H + 14);
    printWrapped(display, card.keyword, y + 10);
  }
}
