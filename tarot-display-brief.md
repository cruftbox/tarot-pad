# E-Ink Tarot Display: Project Brief

A small, battery-powered device that displays a tarot card on an e-ink screen.
The card changes on a timer or on a button press. Two identical units are being
built.

## Hardware (purchased, in hand)

| Part | PID | Notes |
|---|---|---|
| Adafruit ESP32-S3 Feather, 4MB Flash / 2MB PSRAM | 5477 | USB-C, onboard LiPo charging, MAX17048 fuel gauge on I2C |
| Adafruit 2.9" Grayscale eInk FeatherWing, SSD1680 | 4777 | 296x128, microSD socket, onboard SRAM, 3 buttons |
| Lithium Ion Polymer Battery, 3.7V 400mAh | 3898 | JST-PH, sized to sit under a Feather |
| Short Feather Male Headers, 12-pin + 16-pin | 3002 | Soldered to the Feather; the Wing ships with sockets |
| microSD card | n/a | 4GB to 32GB, must be FAT32 |

Quantity 2 of everything. Total hardware cost is roughly $100 for both units.

### Mechanical

- FeatherWing PCB: 79.5mm x 38mm x 6.8mm, excluding mounting tabs
- Mounting holes: 2.5mm diameter on a 74.3mm x 42mm pattern (M2.5 hardware)
- Feather: 52.3mm x 22.7mm x 7.2mm, mounts to the back of the Wing
- Buttons and SD slot are on opposite edges, both on the display-facing side
- Enclosure will be 3D printed. Aesthetic direction is wood-look or brass
  metallic PLA. Final material TBD; prototype in cheap PLA first.

### Display characteristics

- 296 x 128 pixels, SSD1680 controller
- Monochrome refresh is roughly 1 second
- 4-level grayscale is roughly 3 seconds, and Adafruit states plainly that the
  SSD1680 grayscale is grainier than the discontinued ILI0373 it replaced
- Working assumption: use 1-bit monochrome. Rider-Waite-Smith art is heavy black
  linework over flat fills, which thresholds cleanly. Grayscale is likely to
  muddy the outlines rather than add detail. Test both before deciding.

## Orientation and layout

The panel is 296x128, a 2.3:1 landscape ratio. The device will be held portrait,
giving a 128 wide by 296 tall canvas.

Proposed layout, matching the reference device that inspired this project:

```
+------------------+  128 px wide
|                  |
|   card artwork   |  ~224 px tall
|                  |
+------------------+
|  Seven of        |
|  Pentacles       |  ~72 px for title
|  Patience        |  and keyword
+------------------+
```

Render the card name and keyword as text at runtime from a font asset rather
than baking them into each bitmap. One font, 78 short strings, instead of 78
images carrying their own typography.

## Software requirements

### Core behavior

1. Wake from deep sleep on either a timer or a button press
2. Pick a card (random, or sequential, or daily-deterministic)
3. Load the bitmap from microSD, draw it, draw the name and keyword
4. Return to deep sleep

### Settings interface

Three buttons (A, B, C) drive a simple menu. Proposed mapping: A scrolls, B
selects, C backs out. Settings to support:

- Deck selection (each deck is a folder on the SD card)
- Refresh interval
- Card selection mode (random / sequential / daily)

### Power

Target is weeks to months between charges. The ESP32-S3 Feather is specified at
roughly 100uA in deep sleep. E-ink draws current only during refresh and holds
the image at zero power.

The MAX17048 fuel gauge reports charge percentage over I2C, so battery state can
be displayed without additional hardware.

### Storage

Images live on the microSD card, one folder per deck. This removes any practical
capacity limit. A full 78-card monochrome deck is roughly 400KB.

### Time

Neither board has a real-time clock. A deep sleep timer handles "change every N
hours" without one. Anything involving actual clock time or a time zone requires
pulling NTP over WiFi on wake, which costs power. Treat timezone support as
optional and design for the timer case first.

## Hardware verification

These are the things the datasheets do not settle. Confirm each before building
on top of it.

### Results (unit 1, hwcheck firmware, 2026-09-17)

Verified with `firmware/src/hwcheck` on unit 1 only. Unit 2 has not been run.

1. **Button pin mapping: confirmed.** A = GPIO11, B = GPIO12, C = GPIO13 (the
   D-numbers equal GPIO numbers on this Feather). Buttons are active LOW. Each
   press produced one clean LOW/HIGH transition at 10 ms polling; no bounce was
   observed. The A/B/C labels come from the order the buttons were pressed,
   since the firmware reports only GPIO numbers.
2. **RTC-capable GPIOs: confirmed.** GPIO 11, 12, and 13 are all in the RTC range
   (GPIO 0 through 21), so wake-on-button works as wired.
3. **Pull-ups: internal pull-ups work.** With `INPUT_PULLUP`, all watched pins
   (GPIO 8, 11-18) idle HIGH. Whether the Wing has external pull-ups was not
   tested; keep internal pull-ups enabled.
4. **SD card mounting: confirmed.** The card mounted on the first try without
   reformatting: SDHC, 7454 MB, root directory empty.
5. **FeatherWing SRAM: confirmed.** Write/read at three addresses matched.
6. **Display class: confirmed.** `ThinkInk_290_Grayscale4_FPC7519` with
   `setColorBuffer(1, true)` and `setBlackBuffer(0, true)` works in 4-level
   grayscale. Black text is solid, and a black/dark/light/white ramp shows four
   distinct shades in the correct order. Size-1 (1 px stroke) dark gray text is
   faint. `display()` took 10.8 s, but EPD_BUSY is not connected, so that is a
   fixed library wait, not a measured refresh time.
7. **Fuel gauge: confirmed as MAX17048** at I2C 0x36 (chip id 0xC), the only I2C
   device found. Battery read 0.000 V / 0.0 %, probably because no LiPo was
   attached (not confirmed). Battery readings are still untested.

### Still open

1. **Measured deep sleep current.** Verify against the 100uA spec with the
   display and SD card attached, since the real number drives battery sizing.
2. **Header seating.** PID 3002 short male headers are designed to pair with
   Adafruit's short female headers. The Wing ships with factory-installed
   sockets of unconfirmed height. The SRAM and SD tests show electrical contact
   on unit 1; mechanical fit has not been assessed.
3. **Unit 2.** Run hwcheck on the second unit.
4. **Battery readings.** Re-check the MAX17048 voltage and percentage with a
   LiPo attached.
5. **Real refresh time.** EPD_BUSY is not available, so refresh time can only be
   observed by eye or estimated from the library's fixed wait.

## Toolchain decision

Two viable paths. The storage constraint that originally favored Arduino no
longer applies, since images live on SD rather than in flash.

**Arduino / PlatformIO (C++)**: better low-level control over sleep modes and
peripheral power, mature Adafruit_EPD support, full control of the partition
table. More verbose.

**CircuitPython**: much faster iteration, no compile step, drag-and-drop file
updates. The `alarm` module handles deep sleep. The CIRCUITPY filesystem is only
about 1MB on this board, but that does not matter when images are on SD.

Recommendation: PlatformIO, primarily for power control and because two
identical units benefit from a reproducible build.

## Image pipeline

A separate Python tool, run on the desktop, not on the device.

- Source: Rider-Waite-Smith scans. Public domain in the US (published before
  1928) and the UK (Pamela Colman Smith died 1951). Clean sets are available from
  Wikimedia Commons and the Internet Archive.
- Crop and scale to the target artwork area
- Threshold to 1-bit. Compare a plain threshold against dithering; for this art
  style threshold is expected to win.
- Output either 1-bit BMP or a packed raw format. Raw loads faster and skips BMP
  header parsing on the device.
- Emit a companion metadata file (JSON or CSV) with card names and keywords

---

# Claude Code kickoff prompt

Copy everything below into Claude Code.

---

I'm building a battery-powered e-ink tarot card display. Read the attached
project brief (`tarot-display-brief.md`) for full hardware details and
requirements.

Hardware in hand: Adafruit ESP32-S3 Feather (PID 5477) with an Adafruit 2.9"
Grayscale eInk FeatherWing (PID 4777, SSD1680, 296x128), a 400mAh LiPo, and a
microSD card. I'm building two identical units.

Goal: the device shows one tarot card at a time, in portrait orientation, with
the card artwork on top and the card name plus a keyword rendered as text below.
It changes card on a timer or on a button press, and deep sleeps in between to
get weeks of battery life. Card images live on the microSD card, one folder per
deck.

Start here, in this order:

1. Set up a PlatformIO project targeting the Adafruit ESP32-S3 Feather with the
   Adafruit_EPD, Adafruit_GFX, and SD libraries. Use the ThinkInk class that
   matches this SSD1680 FeatherWing; check the library's own examples rather
   than guessing the class name.

2. Write a hardware verification sketch before anything else. It should:
   - Print the GPIO numbers the Wing's A/B/C buttons actually map to on this
     board, and report whether each is in the ESP32-S3 RTC range (GPIO 0-21)
   - Mount the SD card and list its contents
   - Draw a test pattern to the display
   - Read battery voltage and percentage from the MAX17048
   This answers the open questions in the brief. Do not build on assumptions
   about the button pins.

3. Write the desktop Python image conversion tool. Input is a folder of
   Rider-Waite-Smith card scans, output is device-ready 1-bit images plus a
   metadata file mapping filenames to card names and keywords. Make the
   threshold value a parameter so I can compare results. Support both BMP and a
   packed raw format, and tell me which loads faster on-device once we measure.

4. Then the main firmware: deep sleep with timer and button wake, card
   selection, SD image loading, display rendering with text, and a three-button
   settings menu for deck selection and refresh interval.

Constraints and preferences:
- Measure and verify rather than assuming. Several datasheet claims about this
  hardware combination are unconfirmed, and they are listed in the brief.
- Keep the power path explicit. Every wake should do the minimum work and return
  to sleep.
- I want to be able to add decks by dropping a folder on the SD card without
  reflashing.
- Two identical units, so the build needs to be reproducible.

Start with steps 1 and 2. Don't write the main firmware until we know what the
button pins actually are.
