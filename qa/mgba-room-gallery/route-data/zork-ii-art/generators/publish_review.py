from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
import re,math
root=Path.cwd();out=root/'build/zork-ii-art/re-audit'
font=ImageFont.truetype('/System/Library/Fonts/Supplemental/Arial.ttf',17)
small=ImageFont.truetype('/System/Library/Fonts/Supplemental/Arial.ttf',14)
large=ImageFont.truetype('/System/Library/Fonts/Supplemental/Arial.ttf',25)
background='#172321';ink='#eee0b8'
cases=[('mgba-aquarium-intact','One serpent: swimming'),('mgba-aquarium-dead-serpent','Same serpent: dead floor pose'),('mgba-guarded','Guarded Room: door opens away'),('mgba-workshop','Workshop: earlier hinge restored'),('mgba-safe','Blasted safe with crown and card'),('mgba-teapot-ground','Gazebo: dropped teapot on the floor'),('mgba-cage-trapped','Steel cage: robot still reachable'),('mgba-cage-lifted','Robot rescue: mangled cage'),('mgba-bucket-at-top','Top of well: bucket in the shaft'),('mgba-balloon-in-flight','Airborne basket and cargo'),('mgba-crypt-lamp-off','Crypt: dark with faint F'),('mgba-ending','Ending: descending into the red hall')]
sheet=Image.new('RGB',(1488,1614),background);d=ImageDraw.Draw(sheet)
d.text((16,12),'Zork II — scenery and state update',font=large,fill=ink)
d.text((16,46),'Native mGBA frames · final cartridge · 240 × 160 enlarged 2× without smoothing',font=small,fill=ink)
for i,(name,title) in enumerate(cases):
 x=16+i%3*492;y=80+i//3*380
 frame=Image.open(out/(name+'.png'));assert frame.size==(240,160)
 sheet.paste(frame.resize((480,320),Image.Resampling.NEAREST),(x,y));d.text((x,y+326),title,font=font,fill=ink)
 d.text((x,y+350),'mGBA · November 17, 2026',font=small,fill='#abc2b8')
sheet.save(root/'zork-ii-updated-preview.png')
entries=re.findall(r'^SHOT (route-\d+-lit) room=(\d+) scene=(\S+) title=(.*?) dark=',(out/'route.log').read_text(),re.M);assert len(entries)==73
sheet=Image.new('RGB',(1984,80+math.ceil(len(entries)/4)*214),background);d=ImageDraw.Draw(sheet)
d.text((16,12),'Zork II — 73 updated first-entry route views',font=large,fill=ink)
d.text((16,47),'Host renderer · production assets and state · actual 240 × 76 crop enlarged 2×',font=small,fill=ink)
for i,(name,room,scene,title) in enumerate(entries):
 x=16+i%4*492;y=80+i//4*214
 sheet.paste(Image.open(out/(name+'-clear.ppm')).crop((0,15,240,91)).resize((480,152),Image.Resampling.NEAREST),(x,y))
 d.text((x,y+158),f'{room} {title}',font=font,fill=ink);d.text((x,y+182),scene,font=small,fill='#abc2b8')
sheet.save(root/'zork-ii-updated-scenes.png')
sheet=Image.new('RGB',(992,790),background);d=ImageDraw.Draw(sheet)
d.text((16,12),'Optional rooms — remaining scenery gaps',font=large,fill=ink)
for i,(name,title) in enumerate([('trophy','63 Trophy Room'),('quarters',"69 Wizard's Quarters"),('volcano_view','117 Volcano View'),('west_viewing','204 West Viewing Room')]):
 x=8+i%2*496;y=65+i//2*354
 sheet.paste(Image.open(out/('extra-'+name+'.ppm')).resize((480,320),Image.Resampling.NEAREST),(x,y));d.text((x,y+326),title,font=font,fill=ink)
sheet.save(root/'zork-ii-remaining-scenes.png')
print('Published native preview, complete route board, and remaining-room board.')
