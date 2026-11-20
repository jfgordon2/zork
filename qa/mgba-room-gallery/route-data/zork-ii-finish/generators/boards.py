from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
root=Path.cwd();out=root/'build/zork-ii-finish/re-audit'
font=ImageFont.truetype('/System/Library/Fonts/Menlo.ttc',15)
heading=ImageFont.truetype('/System/Library/Fonts/Menlo.ttc',22)
bg=(14,18,24);fg=(225,224,207)
def board(entries,path,title,sub,crop=False):
    w,h=480,(152 if crop else 320);cell=h+46;cols=3;top=84
    b=Image.new('RGB',(cols*w,top+cell*((len(entries)+cols-1)//cols)),bg);d=ImageDraw.Draw(b)
    d.text((16,13),title,font=heading,fill=fg);d.text((16,49),sub,font=font,fill=(162,176,183))
    for i,(label,f,source_crop) in enumerate(entries):
        a=Image.open(f).convert('RGB')
        if source_crop:a=a.crop(source_crop)
        x=i%cols*w;y=top+i//cols*cell
        d.text((x+10,y+10),label,font=font,fill=fg)
        b.paste(a.resize((w,h),Image.Resampling.NEAREST),(x,y+40))
    b.save(path)
def native(name):return out/('mgba-'+name+'.png')
entries=[
('Guarded: same oak door, opens away',native('guarded-open'),None),
('Workshop: right jamb, hidden hinge',native('workshop-open'),None),
('Trophy Room: fixed story scenery',native('trophy'),None),
('Quarters: actual austere description',native('quarters-first'),None),
('Quarters: actual cave description',native('quarters-changed'),None),
('Quarters: restored saved appearance',native('quarters-restored'),None),
('West Viewing: dedicated bank room',native('west-viewing'),None),
('Volcano View: crowded ledge drops',native('volcano-crowded'),None),
('Volcano View: items after return',native('volcano-return'),None),
('Bucket: aboard viewpoint',native('bucket-pov'),None),
('Bucket: outside after Stand up',native('bucket-outside'),None),
('Cerberus: guarding',native('cerberus-guarding'),None),
('Cerberus: friendly, collar attached',native('cerberus-friendly'),None),
('Safe: active fuse spark',native('fuse-spark'),None),
('Safe: original explosion outcome',native('safe-blast'),None),
('Finale: undulating green lines',native('finale-lines'),None),
('Finale: flare on its narrated page',native('finale-flare'),None),
('Finale: lines disappear',native('finale-faded'),None)]
board(entries,root/'zork-ii-completion-preview.png','Zork II — completed follow-ups','Final cartridge | native 240 x 160 mGBA captures | nearest-neighbor 2x')
scene_entries=[('Guarded Room — open',native('guarded-open'),(0,15,240,91)),('Workshop — reverse open view',native('workshop-open'),(0,15,240,91)),('Trophy Room',root/'assets/z2_trophy.png',(0,10,240,86)),('Volcano View — eastern ledge',root/'assets/z2_volcano_view.png',(0,10,240,86)),('West Viewing Room',root/'assets/z2_west_viewing.png',(0,10,240,86))]
for i,label in enumerate(['Cave: furs and old bones','Louis XIV furnishings','1950s bunk bedroom','Jungle hammock','Austere cot and stool','Arabian furnishings'],1):
    scene_entries.append(('Quarters — '+label,out/f'quarters-{i}.ppm',(0,15,240,91)))
scene_entries.append(('Bucket — inside viewpoint',native('bucket-pov'),(0,15,240,91)))
board(scene_entries,root/'zork-ii-completion-scenes.png','Zork II — scenery and connected views','Actual 240 x 76 game crops | six Quarters variants follow the printed story',True)
b=Image.new('RGB',(960,364),bg);d=ImageDraw.Draw(b)
for i,(label,f,_) in enumerate(entries[:2]):
    d.text((i*480+8,10),label,font=font,fill=fg);b.paste(Image.open(f).resize((480,320),Image.Resampling.NEAREST),(i*480,40))
b.save(root/'zork-ii-door-pair.png')
print('Saved three final preview boards.')
