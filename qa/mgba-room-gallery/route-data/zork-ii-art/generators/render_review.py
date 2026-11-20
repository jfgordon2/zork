from PIL import Image,ImageDraw,ImageFont
from pathlib import Path
import re,math
out=Path('build/zork-ii-art/re-audit');font=ImageFont.truetype('/System/Library/Fonts/Supplemental/Arial.ttf',16)
def board(names,file,cols=3):
 sheet=Image.new('RGB',(cols*496,math.ceil(len(names)/cols)*354),'#172321');d=ImageDraw.Draw(sheet)
 for i,name in enumerate(names):
  p=out/(name+'.ppm')
  if not p.exists():print('missing',p);continue
  x=i%cols*496;y=i//cols*354
  sheet.paste(Image.open(p).resize((480,320),Image.Resampling.NEAREST),(x+8,y+8));d.text((x+8,y+330),name,font=font,fill='#eee0b8')
 sheet.save(file)
board(['aquarium-intact','serpent-after-break','gazebo-table','teapot-on-ground','well-floor','bucket-at-top','balloon-ground','balloon-inflated','balloon-air','balloon-moored','cage-trapped','cage-lifted'],out/'states-a.png')
board(['keyhole-open','keyhole-knocked','wooden-door-open','princess-seated','princess-awake','ice-melted','bomb-ready','fuse-burning','dusty-open-safe','crypt-lit','crypt-dark','crypt-secret-open'],out/'states-b.png')
entries=re.findall(r'^SHOT (route-\d+-lit) room=(\d+) scene=(\S+) title=(.*?) dark=',(out/'route.log').read_text(),re.M)
for part in range(math.ceil(len(entries)/16)):
 subset=entries[part*16:(part+1)*16];sheet=Image.new('RGB',(992,math.ceil(len(subset)/2)*200),'#172321');d=ImageDraw.Draw(sheet)
 for i,(name,room,scene,title) in enumerate(subset):
  x=i%2*496;y=i//2*200
  sheet.paste(Image.open(out/(name+'-clear.ppm')).crop((0,15,240,91)).resize((480,152),Image.Resampling.NEAREST),(x+8,y+8));d.text((x+8,y+166),f'{room} {title}: {scene}',font=font,fill='#eee0b8')
 sheet.save(out/f'route-review-{part}.png')
