#!/usr/bin/env python3
"""Stage the static gallery and its directly linked files for GitHub Pages."""

import shutil
import sys
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import unquote, urlsplit


class GalleryLinks(HTMLParser):
    def __init__(self):
        super().__init__()
        self.paths = {Path("index.html")}

    def handle_starttag(self, tag, attrs):
        for name, value in attrs:
            if name not in {"href", "src"} or not value:
                continue
            url = urlsplit(value)
            if not url.scheme and not url.netloc and url.path:
                self.paths.add(Path(unquote(url.path)))


def main(output):
    gallery = Path(__file__).resolve().parent
    links = GalleryLinks()
    links.feed((gallery / "index.html").read_text(encoding="utf-8"))
    output.mkdir()
    for relative in sorted(links.paths):
        source = gallery / relative
        if not source.resolve().is_relative_to(gallery) or not source.is_file():
            raise SystemExit(f"Invalid gallery link: {relative}")
        destination = output / relative
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, destination)
    print(f"Staged {len(links.paths)} gallery files in {output}")


if __name__ == "__main__":
    main(Path(sys.argv[1]))
