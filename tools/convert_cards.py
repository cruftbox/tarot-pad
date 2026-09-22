"""Convert tarot card scans into device-ready 1-bit images for the SD card.

Input is a folder of scans plus a deck CSV (index,name,keyword,source[,titled])
that maps each card to its scan. titled = 1 marks a card whose name is printed
in its artwork, so the device does not draw it again as text.

For every card the tool:

  1. flattens transparency onto white and converts to grayscale: by
     luminance, or with --gray max by each pixel's brightest RGB channel.
     max keeps only ink that is dark in every channel, so hand-stencilled
     color fills drop out and the printed linework stays (Marseille scans).
  2. crops to the card's black frame line (found per card), keeping all
     printed text: numerals and caption bars stay in the image
  3. scales to fit inside the art area and centers it on white, so nothing
     is cropped away
  4. thresholds to 1-bit (or Floyd-Steinberg dithers with --dither)

Output, in out_dir:
  NN.bmp       standard 1-bit BMP
  NN.raw       headerless: rows top to bottom, each row padded to a whole
               byte, MSB = leftmost pixel, 1 = black. This is the layout
               Adafruit_GFX::drawBitmap reads, drawing set bits in the color.
  cards.csv    index,name,keyword,titled for the device
  preview/     NN.png at 3x, and sheet.png with every card at 1x

    python tools/convert_cards.py tools/scans/geldard tools/decks/rws.csv tools/out/rws --threshold 100
"""

import argparse
import csv
import statistics
import sys
from pathlib import Path

from PIL import Image, ImageChops

FRAME_DARK = 100        # gray level below which a pixel counts as frame ink
FRAME_FILL = 0.5        # fraction of a row/column that must be dark to be the frame
FRAME_SEARCH = 0.10     # only search this fraction of each edge for the frame
PREVIEW_SCALE = 3
SHEET_COLUMNS = 13


def load_gray(path, mode="luminance"):
    im = Image.open(path)
    if im.mode in ("RGBA", "LA", "P"):
        im = im.convert("RGBA")
        white = Image.new("RGBA", im.size, (255, 255, 255, 255))
        white.alpha_composite(im)
        im = white
    if mode == "max":
        r, g, b = im.convert("RGB").split()
        return ImageChops.lighter(ImageChops.lighter(r, g), b)
    return im.convert("L")


def find_frame(gray):
    """Return the (left, top, right, bottom) box of the outer frame line, or None."""
    w, h = gray.size
    px = gray.load()

    def is_frame_row(y):
        return sum(px[x, y] < FRAME_DARK for x in range(0, w, 2)) / ((w + 1) // 2) > FRAME_FILL

    def is_frame_col(x):
        return sum(px[x, y] < FRAME_DARK for y in range(0, h, 2)) / ((h + 1) // 2) > FRAME_FILL

    band_h, band_w = int(h * FRAME_SEARCH), int(w * FRAME_SEARCH)
    top = next((y for y in range(band_h) if is_frame_row(y)), None)
    bottom = next((y for y in range(h - 1, h - 1 - band_h, -1) if is_frame_row(y)), None)
    left = next((x for x in range(band_w) if is_frame_col(x)), None)
    right = next((x for x in range(w - 1, w - 1 - band_w, -1) if is_frame_col(x)), None)
    if None in (top, bottom, left, right):
        return None
    return (left, top, right + 1, bottom + 1)


def fit(gray, width, height):
    """Scale to fit inside width x height without cropping; center on white."""
    scale = min(width / gray.width, height / gray.height)
    size = (max(1, round(gray.width * scale)), max(1, round(gray.height * scale)))
    scaled = gray.resize(size, Image.LANCZOS)
    canvas = Image.new("L", (width, height), 255)
    canvas.paste(scaled, ((width - size[0]) // 2, (height - size[1]) // 2))
    return canvas


def to_1bit(gray, threshold, dither):
    if dither:
        return gray.convert("1", dither=Image.Dither.FLOYDSTEINBERG)
    return gray.point(lambda v: 255 if v >= threshold else 0, mode="1")


def raw_bytes(bw):
    # Pillow packs mode "1" MSB-first with byte-padded rows and 1 = white.
    # Invert so 1 = black. Padding bits past the width are ignored by drawBitmap.
    return bytes(b ^ 0xFF for b in bw.tobytes())


def read_deck(path):
    with open(path, newline="", encoding="utf-8") as f:
        rows = list(csv.DictReader(f))
    missing = {"index", "name", "keyword", "source"} - set(rows[0].keys() if rows else ())
    if missing:
        sys.exit(f"{path}: missing columns {sorted(missing)}")
    return rows


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("scans", type=Path, help="folder of source scans")
    ap.add_argument("deck", type=Path, help="deck CSV: index,name,keyword,source[,titled]")
    ap.add_argument("out_dir", type=Path)
    ap.add_argument("--width", type=int, default=128, help="art width in px (default 128)")
    ap.add_argument("--height", type=int, default=224, help="art height in px (default 224)")
    ap.add_argument("--threshold", type=int, default=100,
                    help="gray level 0-255; pixels at or above become white (default 100)")
    ap.add_argument("--gray", choices=["luminance", "max"], default="luminance",
                    help="grayscale from luminance (default) or the brightest RGB channel")
    ap.add_argument("--dither", action="store_true", help="Floyd-Steinberg dither instead of threshold")
    ap.add_argument("--no-frame-crop", action="store_true", help="use the whole scan instead of cropping to the frame")
    args = ap.parse_args()

    cards = read_deck(args.deck)
    missing = [c["source"] for c in cards if not (args.scans / c["source"]).exists()]
    if missing:
        sys.exit(f"{len(missing)} source files not found in {args.scans}, e.g. {missing[0]}")

    preview_dir = args.out_dir / "preview"
    preview_dir.mkdir(parents=True, exist_ok=True)
    sheet_rows = -(-len(cards) // SHEET_COLUMNS)
    sheet = Image.new("L", (SHEET_COLUMNS * (args.width + 4) + 4, sheet_rows * (args.height + 4) + 4), 128)

    aspects = {}
    for n, card in enumerate(cards):
        idx = int(card["index"])
        stem = f"{idx:02d}"
        gray = load_gray(args.scans / card["source"], args.gray)
        if not args.no_frame_crop:
            box = find_frame(gray)
            if box is None:
                print(f"WARNING {stem} {card['name']}: frame not found, using whole scan")
            else:
                gray = gray.crop(box)
                aspects[stem] = (card["name"], gray.width / gray.height)

        bw = to_1bit(fit(gray, args.width, args.height), args.threshold, args.dither)
        bw.save(args.out_dir / f"{stem}.bmp")
        (args.out_dir / f"{stem}.raw").write_bytes(raw_bytes(bw))
        bw.convert("L").resize((args.width * PREVIEW_SCALE, args.height * PREVIEW_SCALE),
                               Image.NEAREST).save(preview_dir / f"{stem}.png")
        col, row = n % SHEET_COLUMNS, n // SHEET_COLUMNS
        sheet.paste(bw.convert("L"), (4 + col * (args.width + 4), 4 + row * (args.height + 4)))

    sheet.save(preview_dir / "sheet.png")
    with open(args.out_dir / "cards.csv", "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["index", "name", "keyword", "titled"])
        for card in cards:
            w.writerow([int(card["index"]), card["name"], card["keyword"], int(card.get("titled") or 0)])

    # A frame box far from the deck's typical shape usually means detection
    # latched onto artwork instead of the frame line.
    if aspects:
        median = statistics.median(a for _, a in aspects.values())
        for stem, (name, a) in sorted(aspects.items()):
            if abs(a - median) / median > 0.02:
                print(f"WARNING {stem} {name}: frame aspect {a:.3f} vs deck median {median:.3f}")

    mode = ("dither" if args.dither else f"threshold {args.threshold}") + f", gray {args.gray}"
    print(f"{len(cards)} cards -> {args.out_dir} ({args.width}x{args.height}, {mode})")


if __name__ == "__main__":
    main()
