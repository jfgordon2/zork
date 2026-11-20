from PIL import Image
from pathlib import Path
im=Image.open("/Users/jeff/.codex/generated_images/01a1e7e8-f99b-7262-a7ec-bbc670193859/exec-0f0068ce-497b-494b-a403-6021f2550257.png").convert('RGBA')
b=im.getchannel('A').point(lambda a:255 if a>=112 else 0).getbbox()
pose=im.crop(b)
pose.thumbnail((91,56),Image.Resampling.LANCZOS)
pose.save('assets/encounter-options/imagegen-sprites/cerberus_friendly.png')
print('Friendly Cerberus preserves source aspect:',pose.size)
