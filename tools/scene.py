from pathlib import Path
import struct
import sys

from PIL import Image


def convert(source, target, flip_horizontal=False):
    with Image.open(source) as image:
        if image.size != (240, 102):
            raise ValueError(f"{source} must be 240 by 102 pixels")
        if flip_horizontal:
            image = image.transpose(Image.Transpose.FLIP_LEFT_RIGHT)
        pixels = image.convert("RGB").getdata()
        gba = [(r >> 3) | (g >> 3) << 5 | (b >> 3) << 10 for r, g, b in pixels]
    Path(target).write_bytes(struct.pack(f"<{len(gba)}H", *gba))


if __name__ == "__main__":
    if len(sys.argv) == 3:
        convert(*sys.argv[1:])
    elif len(sys.argv) == 4 and sys.argv[1] == "--flip-horizontal":
        convert(sys.argv[2], sys.argv[3], flip_horizontal=True)
    else:
        raise SystemExit("usage: scene.py [--flip-horizontal] source.png target.rgb555")
