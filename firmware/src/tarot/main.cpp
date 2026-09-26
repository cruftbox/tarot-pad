// Tarot pad main firmware.
//
//   timer wake  new random card, same deck, at 4 am and every <interval> hours after
//   A           settings (A scrolls, B selects/changes, C exits);
//               B on Clock sets the clock from a phone over a local hotspot
//   B           new random card now
//   C           next deck, same card number
//
// Cards come from the SD card; see common/cards.h for the layout. The chosen
// deck and interval persist in flash; the shuffle survives deep sleep in RTC
// memory. There is no RTC chip: the clock is the ESP32's own, which keeps
// counting through deep sleep but not through a power loss. The first boot
// after a flash sets it from the build machine's local time (build_time.h).
// Without a valid clock the interval counts from the last wake instead.

#include <Arduino.h>
#include <Preferences.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_MAX1704X.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFi.h>
#include <sys/time.h>
#include <time.h>

#include "../common/cards.h"
#include "../common/carddisplay.h"
#include "build_time.h"

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
const int CHANGE_HOUR = 4;  // local time the interval is anchored to
const uint32_t PHONE_TIMEOUT_MS = 300000;

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

bool panelUp = false;  // panel initialised and still powered
bool clockSet = false;  // the ESP32's clock holds local time
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
  panelUp = true;
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

// The clock counts local seconds with no time zone, so gmtime() gives local time.
// A freshly flashed build sets it once; after that it runs on through deep sleep.
// A power loss restarts it near zero, which reads as earlier than the build and
// so as not set.
void initClock() {
  if (prefs.getLong64("built", 0) != BUILD_LOCAL_TIME) {
    struct timeval tv = {(time_t)BUILD_LOCAL_TIME, 0};
    settimeofday(&tv, nullptr);
    prefs.putLong64("built", BUILD_LOCAL_TIME);
    Serial.println("clock set from build");
  }
  clockSet = time(nullptr) >= (time_t)BUILD_LOCAL_TIME;
}

String clockText(time_t t) {
  if (!clockSet) return "not set";
  struct tm tm;
  gmtime_r(&t, &tm);
  char buf[20];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &tm);
  return String(buf);
}

// Seconds from now to the next change: CHANGE_HOUR, then every intervalHours
// after it (so 12 h is 4 am and 4 pm).
uint64_t secondsToNextChange() {
  int64_t period = intervalHours * 3600LL;
  int64_t since = (int64_t)time(nullptr) - CHANGE_HOUR * 3600LL;
  int64_t wait = period - since % period;
  // A timer wake lands on its slot give or take a tick; don't take that slot twice.
  if (wait < 60) wait += period;
  return wait;
}

void sleepNow() {
  prefs.putString("deck", decks[deckIdx]);
  prefs.putUChar("hours", intervalHours);
  prefs.end();
  SD.end();
  display.powerDown();
  panelUp = false;

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
  uint64_t sleepS = clockSet ? secondsToNextChange() : intervalHours * 3600ULL;
  esp_sleep_enable_timer_wakeup(sleepS * 1000000ULL);
  Serial.printf("sleeping; wake on button or in %llu s (at %s)\n", sleepS,
                clockText(time(nullptr) + sleepS).c_str());
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

// The panel stays powered after a draw, so only the first one needs the
// driver's ~1.1 s power-up; a redraw goes straight to the pixels.
void presentMenu() {
  display.setRefreshDelay(MENU_REFRESH_MS);
  if (panelUp) {
    display.displayNoPowerUp();
  } else {
    display.display();
    panelUp = true;
  }
}

void drawSettings(int cursor, const String& battery) {
  const char* labels[] = {"Deck", "Interval", "Clock", "Battery", "Exit"};
  String interval = String(intervalHours) + " h";
  if (clockSet) interval += " from " + String(CHANGE_HOUR) + " am";
  String values[] = {decks[deckIdx], interval, clockText(time(nullptr)), battery, ""};

  display.clearBuffer();
  display.fillScreen(EPD_WHITE);
  display.setTextColor(EPD_BLACK);
  display.setTextSize(2);
  display.setCursor(8, 12);
  display.print("Settings");
  display.setTextSize(1);
  int16_t y = 50;
  for (int i = 0; i < 5; i++) {
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
  unsigned long t0 = millis();
  presentMenu();
  Serial.printf("settings display() %lu ms\n", millis() - t0);
}

// The page sends the phone's local time as seconds since 1970, the same local
// seconds the clock keeps. A form rather than fetch() so it works in the
// stripped-down browsers phones open for a hotspot's sign-in page.
const char PHONE_PAGE[] = R"(<!doctype html>
<meta name=viewport content="width=device-width,initial-scale=1">
<title>Tarot clock</title>
<body style="font-family:sans-serif;text-align:center;padding:2em">
<form action=/set onsubmit="var d=new Date();t.value=Math.floor(d.getTime()/1000)-d.getTimezoneOffset()*60">
<input type=hidden name=t id=t>
<button style="font-size:1.5em;padding:.6em 1.2em">Set tarot clock</button>
</form>)";

void drawPhoneScreen(const String& ssid) {
  display.clearBuffer();
  display.fillScreen(EPD_WHITE);
  display.setTextColor(EPD_BLACK);
  display.setTextSize(2);
  display.setCursor(8, 12);
  display.print("Set clock");
  display.setTextSize(1);
  // 21 characters fill the 128 px width at text size 1.
  const String lines[] = {"On a phone, join", ssid, "", "A page should open.", "If not, browse to",
                          WiFi.softAPIP().toString(), "", "Tap Set tarot clock"};
  int16_t y = 50;
  for (const String& line : lines) {
    display.setCursor(8, y);
    display.print(line);
    y += 14;
  }
  display.setCursor(8, display.height() - 18);
  display.print("C cancel");
  presentMenu();
}

// Runs an open hotspot with a one-button page until the phone sends its time,
// C is pressed, or PHONE_TIMEOUT_MS passes. WiFi is off again on return.
void setClockByPhone() {
  char ssid[16];
  uint64_t mac = ESP.getEfuseMac();
  snprintf(ssid, sizeof(ssid), "Tarot-%02X%02X", (uint8_t)(mac >> 32), (uint8_t)(mac >> 40));
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid);
  Serial.printf("hotspot %s at %s\n", ssid, WiFi.softAPIP().toString().c_str());

  // Answer every name with ourselves, and redirect every unknown path to the
  // page, so the phone's connectivity check finds a sign-in page and opens it.
  DNSServer dns;
  dns.start(53, "*", WiFi.softAPIP());
  WebServer server(80);
  bool done = false;
  uint32_t doneAt = 0;
  server.on("/", [&]() { server.send(200, "text/html", PHONE_PAGE); });
  server.on("/set", [&]() {
    time_t t = server.arg("t").toInt();
    if (t < (time_t)BUILD_LOCAL_TIME) {
      server.send(400, "text/plain", "No time received. Is JavaScript on?");
      return;
    }
    struct timeval tv = {t, 0};
    settimeofday(&tv, nullptr);
    clockSet = true;
    Serial.println("clock set by phone: " + clockText(t));
    server.send(200, "text/html",
                "<meta name=viewport content=\"width=device-width,initial-scale=1\">"
                "<body style=\"font-family:sans-serif;text-align:center;padding:2em\">"
                "<p>Clock set to " + clockText(t) + ".<p>You can leave this network.");
    done = true;
    doneAt = millis();
  });
  server.onNotFound([&]() {
    server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/");
    server.send(302, "text/plain", "");
  });
  server.begin();
  drawPhoneScreen(ssid);

  uint32_t t0 = millis();
  bool wasC = true;  // ignore a C already held
  while (millis() - t0 < PHONE_TIMEOUT_MS) {
    dns.processNextRequest();
    server.handleClient();
    if (done && millis() - doneAt > 2000) break;  // let the reply reach the phone
    bool c = digitalRead(BUTTON_C) == LOW;
    if (c && !wasC) break;
    wasC = c;
    delay(5);
  }
  if (!done) Serial.println("phone clock cancelled or timed out");
  server.stop();
  dns.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
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
      cursor = (cursor + 1) % 5;
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
        setClockByPhone();
        c = true;  // the C that cancelled must not also close settings
      } else if (cursor == 3) {
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
  // Only long enough for a host that is already there: with none attached this
  // waits in full on every wake, and the port turned up within 400 ms on USB.
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 500) delay(10);
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
  initClock();
  Serial.println("clock " + clockText(time(nullptr)));

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
