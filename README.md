# tarot-pad

A small battery-powered e-ink device that shows one tarot card at a time: the
card art on top and its name and a keyword underneath. It draws a new card
on a timer or when you press a button, and stays in deep sleep the rest of
the time. E-ink holds the image with no power, so the card stays on screen
while the board sleeps.

The decks live on a microSD card, one folder per deck. You can add a deck by
copying a folder to the card, with no reflashing.

## Hardware

| Part | Adafruit PID | Notes |
|---|---|---|
| ESP32-S3 Feather, 4 MB Flash / 2 MB PSRAM | 5477 | USB-C, LiPo charging, MAX17048 fuel gauge |
| 2.9" Grayscale eInk FeatherWing (SSD1680, 296 x 128) | 4777 | Has a microSD socket and three buttons |
| Lithium Ion Polymer Battery, 3.7 V 400 mAh | 3898 | JST-PH, fits under the Feather |
| Short Feather Male Headers, 12-pin + 16-pin | 3002 | Soldered to the Feather. The Wing ships with sockets |
| microSD card | n/a | FAT32. This project used 8 GB SDHC cards |

The parts cost roughly $50 per unit. You also need a soldering iron for the
headers, and a 3D printer if you want the case.

### Assembly

1. Solder the short male headers to the Feather.
2. Plug the Feather into the sockets on the back of the Wing.
3. Plug the battery into the Feather's JST connector and tuck it under the
   Feather.
4. Put the SD card in the Wing (see [Preparing the SD card](#preparing-the-sd-card)).

## Using it

The screen is used in portrait, with the USB-C end at the bottom.

| Button | Action |
|---|---|
| A | Settings: A moves the cursor, B changes the selected item, C exits |
| B | Draw a new card now |
| C | Switch to the next deck, keeping the same card |

The settings screen has these items:

- **Deck**: which folder on the SD card to draw from.
- **Interval**: how often a new card is drawn, 6, 12, 24 (the default) or
  48 hours.
- **Battery**: voltage and charge percentage from the fuel gauge.

Settings close on their own after 60 seconds. The deck and interval are saved
in flash and survive a power loss.

The board draws cards from a shuffled order, so all 78 appear before any card
repeats. Neither board has a real-time clock, so "every 24 hours" means 24 hours
since the last wake, not a fixed time of day.

## Firmware

The firmware is built with [PlatformIO](https://platformio.org/). The platform
is pinned in `firmware/platformio.ini` (pioarduino, Arduino-ESP32 core 3.3.7),
so builds are reproducible.

```sh
cd firmware
pio run -t upload            # builds and flashes the main firmware (env: tarot)
pio device monitor           # serial log at 115200
```

**To reflash a unit that is already running**, press A first. In deep sleep the
board's USB serial port disappears, and the settings screen keeps it awake for
60 seconds, which is long enough to start an upload. The COM port number may
change between wakes.

### Other build environments

`firmware/src` also contains small test programs, each with its own env.
Select one with `-e`, for example `pio run -e hwcheck -t upload`.

| Env | What it does |
|---|---|
| `tarot` | The main firmware (default) |
| `hwcheck` | Checks new hardware: prints which GPIOs the buttons use, mounts and lists the SD card, tests the Wing's SRAM, draws a grayscale test pattern, and reads the fuel gauge |
| `deckview` | Demo: A shows a random card, C switches deck |
| `cardview` | Shows one card compiled into the firmware, with no SD card needed |
| `refreshtest` | Tries shorter display refresh waits to find the shortest that still draws cleanly |
| `battest` | Reads the fuel gauge, including its raw registers |

**Run `hwcheck` first on new hardware.** The firmware uses the
`ThinkInk_290_Grayscale4_FPC7519` display class, which is correct for the
panels used in this project. If your Wing has a different panel revision and the
test pattern looks wrong, change the display class.

Pin assignments, confirmed by `hwcheck`: buttons A/B/C on GPIO 11/12/13
(active low, internal pull-ups), SD chip select on 5, EPD chip select on 9, EPD
DC on 10. EPD reset and busy are not connected.

## Preparing the SD card

The firmware expects this layout:

```
/decks/<deck name>/cards.csv     index,name,keyword
/decks/<deck name>/00.raw ... 77.raw
```

Each `.raw` file is 128 x 224 pixels, 1 bit per pixel, with rows top to
bottom, each row padded to a whole byte, the most significant bit leftmost,
and 1 meaning black. The folder name is shown as the deck name in settings.
Decks appear in alphabetical order, and up to 16 are supported.

### Building the decks

The images are generated on your computer with two Python scripts in `tools/`.
You need Python 3.9 or newer and Pillow (`pip install pillow`).

**1. Download the scans** from Wikimedia Commons. `fetch_scans.py` downloads
every file in a category at 1280 px wide, and waits and retries when
Commons rate-limits it.

```sh
python tools/fetch_scans.py "Rider-Waite-Smith tarot deck (Geldard)" tools/scans/geldard
python tools/fetch_scans.py "Vectorized Tarot by Immanuelle" tools/scans/vectorized
python tools/fetch_scans.py "Sola-Busca tarot deck" tools/scans/solabusca
```

**2. Convert them.** Each deck has a CSV in `tools/decks/` that maps card
indices 0-77 to a scan filename, a name and a keyword. The converter crops
each scan to the card's frame, fits it into 128 x 224 without cutting off any
printed titles or numerals, and thresholds it to black and white.

```sh
python tools/convert_cards.py tools/scans/geldard    tools/decks/rws.csv        tools/out/rws        --threshold 100
python tools/convert_cards.py tools/scans/vectorized tools/decks/vectorized.csv tools/out/vectorized --threshold 100
python tools/convert_cards.py tools/scans/solabusca  tools/decks/solabusca.csv  tools/out/solabusca  --threshold 140
```

Each output folder gets the `.raw` files and `cards.csv` for the device, plus
`.bmp` copies and a `preview/` folder with a contact sheet (`sheet.png`) for
checking the results. For the Sola Busca deck the converter warns that it
cannot find the frame on some scans, and uses the whole scan for those cards.

**3. Copy to the card.** Copy each deck's `*.raw` and `cards.csv` into
`/decks/<deck name>/` on the SD card, for example `tools/out/rws` to
`/decks/rws`.

### The included decks

- **rws**: the Rider-Waite-Smith deck, from the Geldard scans on Commons. Its
  keywords are one upright meaning per card. Edit `tools/decks/rws.csv` to
  change them.
- **vectorized**: Immanuelle's vectorized redraw of the Rider-Waite-Smith deck,
  with the same names and keywords.
- **solabusca**: the Sola Busca deck. The trumps keep the names printed on
  them (Mato, Panfilio, ...), and since they have no established meanings,
  their keyword is the trump number. The minor arcana keep the deck's own suits
  in English (Batons, Cups, Swords, Coins) and use the RWS keywords. The suits
  are mapped onto the RWS suits (Batons to Wands, Coins to Pentacles), so
  pressing C shows the corresponding card in each deck.

### Adding your own deck

1. Put the scans in a folder.
2. Write a CSV with the header `index,name,keyword,source`: one row per card,
   where `source` is the scan's filename. Keep indices 0-77 in RWS order (0-21
   major arcana, then Wands, Cups, Swords and Pentacles, Ace to King) if you
   want C to land on the matching card.
3. Run `convert_cards.py` and check `preview/sheet.png`. Adjust `--threshold`
   (a higher value makes the image darker) or try `--dither`. If frame detection
   crops badly, use `--no-frame-crop`.

## Case and stand

`cad/case.scad` is a parametric OpenSCAD model. It has a handheld case, made of
a front shell and a back shell that snap together, and a desk stand that the
case slides into. The front is plain. The back shell and the stand have tarot
motifs engraved 0.6 mm deep, which print without supports.

Ready-to-print STLs are in `cad/stl/`:

| File | Part |
|---|---|
| `front.stl` | Front shell, with the display window |
| `back.stl` | Back shell, with the USB-C opening and the three button holes |
| `plungers.stl` | Three button plungers |
| `stand.stl` | Desk stand, which holds the case leaning back 20° |

To regenerate a part after changing parameters:

```sh
openscad -o cad/stl/back.stl -D 'part="back"' cad/case.scad
```

`part` can be `front`, `back`, `stand`, `plungers`, or `all`, which shows every
part laid out.

### Assembling the case

1. Put the plungers into the button holes from the inside. Hold them in place
   with a piece of adhesive tape across the holes on the outside of the case.
   Nothing else holds them in until the front is on.
2. Mount the board in the back shell with the display facing out, the buttons
   against the plungers and USB-C at its opening.
3. Press the front shell on until the clips snap.
4. Remove the tape.

The case has openings only for USB-C and the three buttons. To change the SD
card, open the case.

The stand holds the case with the USB-C end down, so the unit has to come out of
the stand to charge.

### Fit notes

The model's dimensions were measured from real boards and checked with test
prints. The ones most likely to need adjusting for your printer or boards are
these:

- `fit`: the clearance around the PCB, 1.5 mm.
- `stand_slop`: the clearance between the case and the stand slot, 1.0 mm.
- `usb_from_glass` and `usb_from_tab_edge`: the position of the USB-C opening.
  The values in the file were adjusted from test prints.
- `glass_t`: the thickness of the display glass. It is an estimate (1.2 mm)
  and sets the height of the posts that support the board. If the board sits
  loose or proud at the far end, adjust this value.

## Troubleshooting

- **"SD card not found"** on screen: the card did not mount. Check that it is
  FAT32 and fully inserted.
- **"No decks in /decks"**: the card mounted but has no deck folders at
  `/decks/<name>/`.
- **Card name shows with blank art**: the deck's `NN.raw` file for that card
  is missing. The serial log reports `[art missing]`.
- **Upload can't find the port**: the board is asleep. Press A and upload
  within 60 seconds.

## License

MIT, see [LICENSE](LICENSE). The card scans come from Wikimedia Commons and are
not included in this repository. Check each file's license on Commons before
redistributing converted decks.
