"""Import a generated image at the game's native background or sprite size.

Backgrounds:  python3 tools/import_art.py SOURCE NAME --game 2   -> assets/z2_NAME.png
Sprite pose:  python3 tools/import_art.py SOURCE NAME --sprite POSE WIDTH HEIGHT
              (adds or replaces POSE on assets/items/NAME.png and frames.json)
"""
import argparse
from pathlib import Path

from PIL import Image

from item_import import crop_to_scene_aspect, import_sprite


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("source", type=Path)
    parser.add_argument("name")
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--game", type=int, choices=(1, 2, 3), help="background for Zork I, II or III")
    group.add_argument("--sprite", nargs=3, metavar=("POSE", "WIDTH", "HEIGHT"))
    args = parser.parse_args()
    assets = Path(__file__).resolve().parents[1] / "assets"
    image = Image.open(args.source).convert("RGBA")
    if args.sprite:
        pose, width, height = args.sprite
        import_sprite(image, assets, args.name, pose, int(width), int(height))
    else:
        crop_to_scene_aspect(image).convert("RGB").resize((240, 102), Image.Resampling.LANCZOS) \
            .save(assets / f"z{args.game}_{args.name}.png")


if __name__ == "__main__":
    main()
