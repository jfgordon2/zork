"""Shared crop and sprite-sheet helpers for tools/import_art.py."""
import fcntl
import json

from PIL import Image

ASPECT = 240 / 102


def crop_to_scene_aspect(image):
    """Centre-crop to the 240:102 scene shape so resizing never squashes the art."""
    width, height = image.size
    if width / height > ASPECT:
        crop = round(height * ASPECT)
        left = (width - crop) // 2
        return image.crop((left, 0, left + crop, height))
    crop = round(width / ASPECT)
    top = (height - crop) // 2
    return image.crop((0, top, width, top + crop))


def import_sprite(image, assets, name, pose, width, height):
    alpha = image.getchannel('A')
    bounds = alpha.point(lambda a: 255 if a >= 112 else 0).getbbox()
    if bounds is None or alpha.getextrema()[0] == 255:
        raise ValueError('The generated sprite must have a real transparent background')
    image = image.crop(bounds)
    image.thumbnail((width, height), Image.Resampling.LANCZOS)
    cell_image = Image.new('RGBA', (width, height))
    cell_image.alpha_composite(image, ((width - image.width) // 2, height - image.height))

    sheet_path = assets / 'items' / f'{name}.png'
    manifest = assets / 'items/frames.json'
    lock_path = assets.parent / 'build/item-import.lock'
    lock_path.parent.mkdir(parents=True, exist_ok=True)
    with lock_path.open('a') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        frames = json.loads(manifest.read_text())
        cells = frames.get(name, {}).get('cells', [])
        existing = next((c for c in cells if c['pose'] == pose), None)
        if cells and sheet_path.exists():
            sheet = Image.open(sheet_path).convert('RGBA')
            if existing:
                # Replace this pose in place; other poses on the sheet are untouched.
                if (existing['width'], existing['height']) != (width, height):
                    raise ValueError(f"{name} {pose} is {existing['width']}x{existing['height']}; "
                                     f"import it at that size or remove the pose first")
                x, y = existing['x'], existing['y']
                sheet.paste(Image.new('RGBA', (width, height)), (x, y))
                sheet.paste(cell_image, (x, y))
            else:
                # Append a new pose to the right of the existing poses.
                x, y = sheet.width + 2, 0
                grown = Image.new('RGBA', (x + width, max(sheet.height, height)))
                grown.paste(sheet, (0, 0))
                grown.paste(cell_image, (x, y))
                sheet = grown
                cells.append({'pose': pose, 'x': x, 'y': y, 'width': width, 'height': height})
        else:
            sheet = cell_image
            cells = [{'pose': pose, 'x': 0, 'y': 0, 'width': width, 'height': height}]
        sheet.save(sheet_path)
        entry = frames.get(name, {})
        entry['cells'] = cells
        frames[name] = entry
        manifest.write_text(json.dumps(frames, indent=2) + '\n')
