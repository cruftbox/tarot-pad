// Tarot pad main firmware.
//
//   timer wake  new random card, same deck, every <interval> hours
//   A           settings (A scrolls, B selects/changes, C exits)
//   B           new random card now
//   C           next deck, same card number
//
// Cards come from the SD card; see common/cards.h for the layout. The chosen
// deck and interval persist in flash; the shuffle survives deep sleep in RTC
// memory. No RTC chip on either board, so "once a day" is a 24 h sleep timer
// measured from the last wake, not wall-clock time.

#include <Arduino.h>
#include <Preferences.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_MAX1704X.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>

#include "../common/cards.h"

// FeatherWing pins, as verified by hwcheck
#define SD_CS     5
#define EPD_CS    9
#define EPD_DC    10
#define EPD_RESET -1
#define EPD_BUSY  -1

const gpio_num_t BUTTON_A = GPIO_NUM_11;  // settings
const gpio_num_t BUTTON_B = GPIO_NUM_12;  // new card
const gpio_num_t BUTTON_C = GPIO_NUM_13;  // next deck

const uint8_t ROTATION = 3;            // card top at the red FPC tab end
const uint16_t REFRESH_DELAY_MS = 2000;   // see refreshtest; driver default is 9000
// Menus are plain black text, the easiest case for the panel, so they get a
// shorter wait than the cards.
const uint16_t MENU_REFRESH_MS = 500;
const int MAX_DECKS = 16;
const int MAX_CARDS = 78;
const uint8_t INTERVALS[] = {6, 12, 24, 48};
const int INTERVAL_COUNT = sizeof(INTERVALS) / sizeof(INTERVALS[0]);
const uint8_t DEFAULT_INTERVAL = 24;
const uint32_t SETTINGS_TIMEOUT_MS = 60000;
const uint32_t SHUFFLE_MAGIC = 0x7A401;

// default_refresh_delay is protected in the driver.
class CardDisplay : public ThinkInk_290_Grayscale4_FPC7519 {
 public:
  using ThinkInk_290_Grayscale4_FPC7519::ThinkInk_290_Grayscale4_FPC7519;
  void setRefreshDelay(uint16_t ms) { default_refresh_delay = ms; }
};

// Framebuffer in ESP32 RAM (SRAM CS = -1): drawing via the Wing's SPI SRAM cost
// four SPI transactions per pixel, ~3.9 s for a full screen.
CardDisplay display(EPD_DC, EPD_RESET, EPD_CS, -1, EPD_BUSY);
Adafruit_MAX17048 maxlipo;
Preferences prefs;

// Survives deep sleep, lost on power loss.
RTC_DATA_ATTR uint32_t rtcMagic = 0;
RTC_DATA_ATTR uint8_t rtcOrder[MAX_CARDS];
RTC_DATA_ATTR uint8_t rtcPos = 0;
RTC_DATA_ATTR uint8_t rtcCard = 0;

String decks[MAX_DECKS];
int deckCount = 0;
int deckIdx = 0;
uint8_t intervalHours = DEFAULT_INTERVAL;
int cardCount = MAX_CARDS;
uint8_t art[ART_BYTES];

void reshuffle() {
  for (int i = 0; i < cardCount; i++) rtcOrder[i] = i;
  for (int i = cardCount - 1; i > 0; i--) {  // Fisher-Yates
    int j = esp_random() % (i + 1);
    std::swap(rtcOrder[i], rtcOrder[j]);
  }
  rtcPos = 0;
  rtcMagic = SHUFFLE_MAGIC;
}

// Next card in the shuffled order; every card appears before any repeats.
uint8_t nextCard() {
  if (rtcMagic != SHUFFLE_MAGIC || rtcPos >= cardCount) reshuffle();
  return rtcOrder[rtcPos++];
}

void showCard() {
  Serial.printf("[%lu ms] showCard\n", millis());
  Card card;
  int count = readCards(decks[deckIdx], rtcCard, card);
  if (count > 0) cardCount = min(count, MAX_CARDS);
  bool artOk = readArt(decks[deckIdx], rtcCard, art);
  Serial.printf("deck %s card %02d: %s / %s%s%s\n", decks[deckIdx].c_str(), rtcCard,
                card.name.c_str(), card.keyword.c_str(), card.titled ? "  [titled]" : "",
                artOk ? "" : "  [art missing]");
  drawCard(display, art, artOk, card);
  display.setRefreshDelay(REFRESH_DELAY_MS);
  unsigned long t0 = millis();
  display.display();
  Serial.printf("[%lu ms] card display() %lu ms\n", millis(), millis() - t0);
}

void showMessage(const char* msg) {
  display.clearBuffer();
  display.fillScreen(EPD_WHITE);
  display.setTextColor(EPD_BLACK);
  display.setTextSize(1);
  printWrapped(display, msg, 140);
  display.display();
}

void sleepNow() {
  prefs.putString("deck", decks[deckIdx]);
  prefs.putUChar("hours", intervalHours);
  prefs.end();
  SD.end();
  display.powerDown();

  // Buttons have no external pull-ups, so hold them up through deep sleep or
  // the floating pins wake the board immediately.
  for (gpio_num_t pin : {BUTTON_A, BUTTON_B, BUTTON_C}) {
    rtc_gpio_pullup_en(pin);
    rtc_gpio_pulldown_dis(pin);
  }
  // Don't sleep with a button still held: that counts as a new press.
  uint32_t t0 = millis();
  while (millis() - t0 < 5000 &&
         (digitalRead(BUTTON_A) == LOW || digitalRead(BUTTON_B) == LOW || digitalRead(BUTTON_C) == LOW))
    delay(20);

  uint64_t mask = (1ULL << BUTTON_A) | (1ULL << BUTTON_B) | (1ULL << BUTTON_C);
  esp_sleep_enable_ext1_wakeup(mask, ESP_EXT1_WAKEUP_ANY_LOW);
  esp_sleep_enable_timer_wakeup((uint64_t)intervalHours * 3600ULL * 1000000ULL);
  Serial.printf("sleeping; wake on button or in %u h\n", intervalHours);
  Serial.flush();
  esp_deep_sleep_start();
}

String batteryText() {
  static bool ready = false;
  if (!ready) {
    pinMode(PIN_I2C_POWER, OUTPUT);
    digitalWrite(PIN_I2C_POWER, HIGH);
    delay(10);
    Wire.begin();
    if (!maxlipo.begin()) return "n/a";
    // begin() resets the gauge and the first VCELL read comes back 0x0000;
    // the next measurement lands within about a second (battest, 2026-09-17).
    uint32_t t0 = millis();
    while (maxlipo.cellVoltage() < 0.1f && millis() - t0 < 1500) delay(50);
    ready = true;
  }
  float volts = maxlipo.cellVoltage();
  if (volts < 0.1f) return "n/a";
  char buf[32];
  snprintf(buf, sizeof(buf), "%.2fV  %.0f%%", volts, min(maxlipo.cellPercent(), 100.0f));
  return String(buf);
}

void drawSettings(int cursor, const String& battery) {
  const char* labels[] = {"Deck", "Interval", "Battery", "Exit"};
  String values[] = {decks[deckIdx], String(intervalHours) + " h", battery, ""};

  display.clearBuffer();
  display.fillScreen(EPD_WHITE);
  display.setTextColor(EPD_BLACK);
  display.setTextSize(2);
  display.setCursor(8, 12);
  display.print("Settings");
  display.setTextSize(1);
  int16_t y = 50;
  for (int i = 0; i < 4; i++) {
    display.setCursor(8, y);
    display.print(i == cursor ? ">" : " ");
    display.print(labels[i]);
    y += 12;
    if (values[i].length()) {
      display.setCursor(24, y);
      display.print(values[i]);
      y += 12;
    }
    y += 8;
  }
  display.setCursor(8, display.height() - 30);
  display.print("A move  B change");
  display.setCursor(8, display.height() - 18);
  display.print("C exit");
  display.setRefreshDelay(MENU_REFRESH_MS);
  unsigned long t0 = millis();
  display.display();
  Serial.printf("settings display() %lu ms\n", millis() - t0);
}

void settingsScreen() {
  int cursor = 0;
  String battery = batteryText();
  drawSettings(cursor, battery);
  uint32_t last = millis();
  bool wasA = false, wasB = false, wasC = false;

  while (millis() - last < SETTINGS_TIMEOUT_MS) {
    // Stay awake while a host has the serial port open (DTR set, e.g. a
    // serial monitor), so the port stays up for flashing and logs.
    if (Serial) last = millis();
    bool a = digitalRead(BUTTON_A) == LOW, b = digitalRead(BUTTON_B) == LOW, c = digitalRead(BUTTON_C) == LOW;
    if (c && !wasC) return;
    if (a && !wasA) {
      cursor = (cursor + 1) % 4;
      drawSettings(cursor, battery);
      last = millis();
    } else if (b && !wasB) {
      if (cursor == 0) {
        deckIdx = (deckIdx + 1) % deckCount;
      } else if (cursor == 1) {
        int i = 0;
        while (i < INTERVAL_COUNT - 1 && INTERVALS[i] != intervalHours) i++;
        intervalHours = INTERVALS[(i + 1) % INTERVAL_COUNT];
      } else if (cursor == 2) {
        battery = batteryText();
      } else {
        return;
      }
      drawSettings(cursor, battery);
      last = millis();
    }
    wasA = a;
    wasB = b;
    wasC = c;
    delay(20);
  }
  Serial.println("settings timed out");
}

void setup() {
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 2000) delay(10);
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);
  for (gpio_num_t pin : {BUTTON_A, BUTTON_B, BUTTON_C}) rtc_gpio_hold_dis(pin);

  Serial.printf("[%lu ms] boot\n", millis());
  SPI.begin();
  display.begin(THINKINK_GRAYSCALE4);
  Serial.printf("[%lu ms] display.begin done\n", millis());
  display.setRefreshDelay(REFRESH_DELAY_MS);
  display.setColorBuffer(1, true);
  display.setBlackBuffer(0, true);
  display.setRotation(ROTATION);

  prefs.begin("tarot", false);
  intervalHours = prefs.getUChar("hours", DEFAULT_INTERVAL);
  String savedDeck = prefs.getString("deck", "");

  if (!SD.begin(SD_CS, SPI, 4000000)) {
    Serial.println("SD.begin failed");
    showMessage("SD card not found");
    sleepNow();
  }
  deckCount = findDecks(decks, MAX_DECKS);
  if (deckCount == 0) {
    Serial.println("no decks in /decks");
    showMessage("No decks in /decks");
    sleepNow();
  }
  for (int i = 0; i < deckCount; i++)
    if (decks[i] == savedDeck) deckIdx = i;
  Serial.printf("[%lu ms] SD ready\n", millis());
  for (int i = 0; i < deckCount; i++) Serial.printf("deck %d: %s\n", i, decks[i].c_str());

  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  uint64_t pins = (cause == ESP_SLEEP_WAKEUP_EXT1) ? esp_sleep_get_ext1_wakeup_status() : 0;
  bool wokeA = pins & (1ULL << BUTTON_A);
  bool wokeB = pins & (1ULL << BUTTON_B);
  bool wokeC = pins & (1ULL << BUTTON_C);
  Serial.printf("wake cause %d%s%s%s\n", cause, wokeA ? " A" : "", wokeB ? " B" : "", wokeC ? " C" : "");
  Serial.println("battery " + batteryText());
  Serial.printf("[%lu ms] battery read\n", millis());

  if (wokeA) {
    settingsScreen();
    showCard();
  } else if (wokeC) {
    deckIdx = (deckIdx + 1) % deckCount;  // same card number in the next deck
    showCard();
  } else if (wokeB) {
    rtcCard = nextCard();
    showCard();
  } else {
    // Timer wake, reset, or power-on: a new card.
    rtcCard = nextCard();
    showCard();
  }
  sleepNow();
}

void loop() {}  // setup() always ends in deep sleep
