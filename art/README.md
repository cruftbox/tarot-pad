# Artwork

Each folder holds the 78 cards of one deck as `00.png` to `77.png`, in RWS
order (0-21 major arcana, then Wands, Cups, Swords and Pentacles, Ace to King),
matching the `index` column in `tools/decks/<deck>.csv`. `sheet.png` shows the
whole deck on one page.

- **rws**, the Rider-Waite-Smith deck from the Geldard scans, as the device
  shows it: the converted 1-bit images, enlarged 2x.
  [Cards](rws/) · [Sheet](rws/sheet.png)
- **black_ink**, traditional tarot imagery in a bold woodcut style.
  [Cards](black_ink/) · [Sheet](black_ink/sheet.png)
- **hundred-acre**, a storybook deck of animal characters from the
  Winnie-the-Pooh stories. [Cards](hundred-acre/) · [Sheet](hundred-acre/sheet.png)
- **wonderland**, Alice in Wonderland in the style of the Victorian
  illustrations. [Cards](wonderland/) · [Sheet](wonderland/sheet.png)

The black_ink, hundred-acre and wonderland cards are the original art, reduced
to 474 x 830. The device shows them as 1-bit images at 128 x 224.

These files are built with `tools/build_art.py`:

```
python tools/build_art.py tools/decks/rws.csv art/rws --raw tools/out/rws --scale 2
python tools/build_art.py tools/decks/black_ink.csv art/black_ink --png tools/scans/black_ink
```
