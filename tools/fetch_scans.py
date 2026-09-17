"""Download every file in a Wikimedia Commons category as a scaled rendition.

Commons serves thumbnails at a requested width, which avoids pulling the
multi-megabyte originals when the device image is only 128 px wide.

    python tools/fetch_scans.py "Rider-Waite-Smith tarot deck (Geldard)" tools/scans/geldard
"""

import argparse
import json
import re
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path

API = "https://commons.wikimedia.org/w/api.php"
# Wikimedia requires a descriptive User-Agent for API and upload.wikimedia.org requests.
USER_AGENT = "tarot-pad-fetch-scans/0.1 (https://github.com/cruftbox/tarot-pad)"


def http_get(url, retries=6):
    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    for attempt in range(retries):
        try:
            with urllib.request.urlopen(req, timeout=60) as resp:
                return resp.read()
        except urllib.error.HTTPError as e:
            if e.code != 429 or attempt == retries - 1:
                raise
            wait = int(e.headers.get("Retry-After") or 0) or 15 * 2 ** attempt
            print(f"  rate limited (429); waiting {wait} s")
            time.sleep(wait)


def api(**params):
    params["format"] = "json"
    return json.loads(http_get(API + "?" + urllib.parse.urlencode(params)))


def local_name(title):
    # Commons titles can contain characters Windows forbids in filenames.
    return re.sub(r'[<>:"/\|?*]', "_", title.removeprefix("File:"))


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


def thumb_urls(titles, width):
    # Queried by explicit titles, 50 at a time: with a generator, Commons omits
    # imageinfo for some pages when a scaled URL is requested.
    urls = {}
    for i in range(0, len(titles), 50):
        data = api(action="query", titles="|".join(titles[i:i + 50]), prop="imageinfo",
                   iiprop="url", iiurlwidth=width)
        for page in data["query"]["pages"].values():
            if "imageinfo" in page:
                urls[page["title"]] = page["imageinfo"][0]["thumburl"]
        time.sleep(1)
    return urls


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("category", help="Commons category name, without the 'Category:' prefix")
    ap.add_argument("out_dir", type=Path)
    ap.add_argument("--width", type=int, default=1280, help="rendition width in px (default 1280)")
    ap.add_argument("--match", help="only download files whose name contains this text")
    ap.add_argument("--delay", type=float, default=1.5, help="seconds between downloads (default 1.5)")
    args = ap.parse_args()

    args.out_dir.mkdir(parents=True, exist_ok=True)
    titles = category_files(args.category)
    if args.match:
        titles = [t for t in titles if args.match in t]
    if not titles:
        sys.exit(f"no matching files found in Category:{args.category}")
    print(f"{len(titles)} files to fetch from Category:{args.category}")

    todo = [t for t in titles if not (args.out_dir / local_name(t)).exists()]
    urls = thumb_urls(todo, args.width) if todo else {}
    for i, title in enumerate(titles, 1):
        name = local_name(title)
        dest = args.out_dir / name
        if dest.exists():
            print(f"[{i}/{len(titles)}] have {name}")
            continue
        if title not in urls:
            print(f"[{i}/{len(titles)}] SKIP {name}: no URL returned")
            continue
        data = http_get(urls[title])
        dest.write_bytes(data)
        print(f"[{i}/{len(titles)}] got  {name} ({len(data) // 1024} KB)")
        time.sleep(args.delay)  # be polite to Commons


if __name__ == "__main__":
    main()
