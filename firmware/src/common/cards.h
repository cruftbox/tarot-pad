// Deck storage on the SD card and the portrait card layout.
//
// SD layout, as written by tools/convert_cards.py:
//   /decks/<deck>/NN.raw     128x224, 1 = black, MSB first, byte-padded rows
//   /decks/<deck>/cards.csv  index,name,keyword
#pragma once

#include <Arduino.h>
#include "Adafruit_ThinkInk.h"

const uint16_t ART_W = 128;
const uint16_t ART_H = 224;
const size_t ART_BYTES = ((ART_W + 7) / 8) * ART_H;

struct Card {
  String name;
  String keyword;
};

// Deck folder names found under /decks, sorted; returns how many were stored.
int findDecks(String* decks, int maxDecks);

// Number of rows in the deck's cards.csv; fills `card` if `index` is present.
int readCards(const String& deck, int index, Card& card);

// Reads ART_BYTES of NN.raw into `buf`.
bool readArt(const String& deck, int index, uint8_t* buf);

// Draws text centered at `y`, wrapping at spaces; returns the next free y.
int16_t printWrapped(Adafruit_EPD& display, const String& text, int16_t y);

// Card art in the top ART_H rows, name and keyword below. Does not refresh.
void drawCard(Adafruit_EPD& display, const uint8_t* art, bool artOk, const Card& card);
