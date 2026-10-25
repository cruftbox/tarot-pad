"""Convert device .raw card images back into PNGs for viewing.

A .raw file is the device format written by convert_cards.py: rows top to
bottom, each row padded to a whole byte, MSB = leftmost pixel, 1 = black.

    python tools/raw_to_png.py tools/out/rws preview_dir --scale 2
"""

import argparse
from pathlib import Path

from PIL import Image


def read_raw(path, width=128, height=224):
    """Return the .raw file at path as a mode "1" image."""
    data = Path(path).read_bytes()
    expected = (width + 7) // 8 * height
    if len(data) != expected:
        raise ValueError(f"{path}: {len(data)} bytes, expected {expected} for {width}x{height}")
    # Pillow's "1" raw decoder uses the same packing with 1 = white, so invert.
    return Image.frombytes("1", (width, height), bytes(b ^ 0xFF for b in data))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("raw_dir", type=Path, help="folder of NN.raw files")
    ap.add_argument("out_dir", type=Path)
    ap.add_argument("--width", type=int, default=128, help="art width in px (default 128)")
    ap.add_argument("--height", type=int, default=224, help="art height in px (default 224)")
    ap.add_argument("--scale", type=int, default=1, help="nearest-neighbor upscale factor (default 1)")
    args = ap.parse_args()

    args.out_dir.mkdir(parents=True, exist_ok=True)
    raws = sorted(args.raw_dir.glob("*.raw"))
    for path in raws:
        im = read_raw(path, args.width, args.height).convert("L")
        if args.scale != 1:
            im = im.resize((args.width * args.scale, args.height * args.scale), Image.NEAREST)
        im.save(args.out_dir / f"{path.stem}.png", optimize=True)
    print(f"{len(raws)} cards -> {args.out_dir}")


if __name__ == "__main__":
    main()
