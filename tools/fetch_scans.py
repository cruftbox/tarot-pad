"""Download every file in a Wikimedia Commons category as a scaled rendition.

Commons serves thumbnails at a requested width, which avoids pulling the
multi-megabyte originals when the device image is only 128 px wide.

    python tools/fetch_scans.py "Rider-Waite-Smith tarot deck (Geldard)" tools/scans/geldard
"""

import argparse
import json
import sys
import time
import urllib.parse
import urllib.request
from pathlib import Path

API = "https://commons.wikimedia.org/w/api.php"
# Wikimedia requires a descriptive User-Agent for API and upload.wikimedia.org requests.
USER_AGENT = "tarot-pad-fetch-scans/0.1 (https://github.com/cruftbox/tarot-pad)"


def http_get(url):
    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    with urllib.request.urlopen(req, timeout=60) as resp:
        return resp.read()


def api(**params):
    params["format"] = "json"
    return json.loads(http_get(API + "?" + urllib.parse.urlencode(params)))


def category_files(category):
    titles = []
    params = dict(action="query", list="categorymembers", cmtitle="Category:" + category,
                  cmtype="file", cmlimit=500)
    while True:
        data = api(**params)
        titles += [m["title"] for m in data["query"]["categorymembers"]]
        if "continue" not in data:
            return titles
        params.update(data["continue"])


def thumb_url(title, width):
    # Queried per title: with a generator, Commons omits imageinfo for some pages
    # when a scaled URL is requested.
    data = api(action="query", titles=title, prop="imageinfo", iiprop="url", iiurlwidth=width)
    page = next(iter(data["query"]["pages"].values()))
    return page["imageinfo"][0]["thumburl"]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("category", help="Commons category name, without the 'Category:' prefix")
    ap.add_argument("out_dir", type=Path)
    ap.add_argument("--width", type=int, default=1280, help="rendition width in px (default 1280)")
    args = ap.parse_args()

    args.out_dir.mkdir(parents=True, exist_ok=True)
    titles = category_files(args.category)
    if not titles:
        sys.exit(f"no files found in Category:{args.category}")
    print(f"{len(titles)} files in Category:{args.category}")

    for i, title in enumerate(titles, 1):
        name = title.removeprefix("File:")
        dest = args.out_dir / name
        if dest.exists():
            print(f"[{i}/{len(titles)}] have {name}")
            continue
        data = http_get(thumb_url(title, args.width))
        dest.write_bytes(data)
        print(f"[{i}/{len(titles)}] got  {name} ({len(data) // 1024} KB)")
        time.sleep(0.5)  # be polite to Commons


if __name__ == "__main__":
    main()
