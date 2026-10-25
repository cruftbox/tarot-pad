"""Build the browsable art/<deck>/ folders: NN.png per card and sheet.png.

Cards come either from the device .raw files (--raw, upscaled with nearest
neighbor so the pixels stay sharp) or from full-size source PNGs (--png,
downscaled to --width). The deck CSV gives the order: the output for index N
is NN.png. sheet.png shows every card in index order on a white gutter, and
README.md shows them as a grid of thumbnails on GitHub.

    python tools/build_art.py tools/decks/rws.csv art/rws --raw tools/out/rws --scale 2
    python tools/build_art.py tools/decks/wonderland.csv art/wonderland --png tools/scans/wonderland --width 474
"""

import argparse
import csv
from pathlib import Path

from PIL import Image

from raw_to_png import read_raw

SHEET_COLUMNS = 13
SHEET_CELL = (128, 224)
GUTTER = 4
README_COLUMNS = 6
README_THUMB = 120


def write_readme(out_dir, cards):
    """Write README.md with a grid of every card, which GitHub shows under the file list."""
    lines = [f"# {out_dir.name}", "", "[Contact sheet](sheet.png)", "", "<table>"]
    for start in range(0, len(cards), README_COLUMNS):
        lines.append("<tr>")
        for card in cards[start:start + README_COLUMNS]:
            stem = f"{int(card['index']):02d}"
            lines.append(f'<td align="center"><a href="{stem}.png"><img src="{stem}.png" '
                         f'width="{README_THUMB}" alt="{card["name"]}"></a><br>{stem} {card["name"]}</td>')
        lines.append("</tr>")
    lines.append("</table>")
    (out_dir / "README.md").write_text("\n".join(lines) + "\n", encoding="utf-8")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("deck", type=Path, help="deck CSV: index,name,keyword,source[,titled]")
    ap.add_argument("out_dir", type=Path)
    src = ap.add_mutually_exclusive_group(required=True)
    src.add_argument("--raw", type=Path, help="folder of device NN.raw files")
    src.add_argument("--png", type=Path, help="folder of source images named in the CSV source column")
    ap.add_argument("--scale", type=int, default=2, help="upscale factor for --raw (default 2)")
    ap.add_argument("--width", type=int, default=474, help="output width for --png (default 474)")
    args = ap.parse_args()

    with open(args.deck, newline="", encoding="utf-8") as f:
        cards = sorted(csv.DictReader(f), key=lambda c: int(c["index"]))

    args.out_dir.mkdir(parents=True, exist_ok=True)
    rows = -(-len(cards) // SHEET_COLUMNS)
    cw, ch = SHEET_CELL
    sheet = Image.new("RGB", (SHEET_COLUMNS * (cw + GUTTER) + GUTTER, rows * (ch + GUTTER) + GUTTER), "white")

    for n, card in enumerate(cards):
        stem = f"{int(card['index']):02d}"
        if args.raw:
            im = read_raw(args.raw / f"{stem}.raw", *SHEET_CELL).convert("L")
            thumb = im
            im = im.resize((im.width * args.scale, im.height * args.scale), Image.NEAREST)
        else:
            src = Image.open(args.png / card["source"]).convert("RGB")
            im = src.resize((args.width, round(src.height * args.width / src.width)), Image.LANCZOS)
            thumb = src.copy()
            thumb.thumbnail(SHEET_CELL, Image.LANCZOS)
        im.save(args.out_dir / f"{stem}.png", optimize=True)
        col, row = n % SHEET_COLUMNS, n // SHEET_COLUMNS
        x = GUTTER + col * (cw + GUTTER) + (cw - thumb.width) // 2
        y = GUTTER + row * (ch + GUTTER) + (ch - thumb.height) // 2
        sheet.paste(thumb, (x, y))

    sheet.save(args.out_dir / "sheet.png", optimize=True)
    write_readme(args.out_dir, cards)
    print(f"{len(cards)} cards -> {args.out_dir}")


if __name__ == "__main__":
    main()
