from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
import re,math
names=re.findall(r'^ZORK2_SCENE\(([^,]+),',Path('src/zork2_scenes.def').read_text(),re.M)
font=ImageFont.truetype('/System/Library/Fonts/Supplemental/Arial.ttf',15)
for part in range(math.ceil(len(names)/16)):
 group=names[part*16:(part+1)*16];sheet=Image.new('RGB',(992,math.ceil(len(group)/2)*190),'#172321');d=ImageDraw.Draw(sheet)
 for i,name in enumerate(group):
  path=Path('assets/z2_'+name+'.png')
  if not path.exists():continue
  x=i%2*496;y=i//2*190
  art=Image.open(path).crop((0,10,240,86)).resize((480,152),Image.Resampling.NEAREST)
  sheet.paste(art,(x+8,y+8));d.text((x+8,y+162),name,font=font,fill='#eee0b8')
 sheet.save(f'build/zork-ii-art/crops-{part}.png')
