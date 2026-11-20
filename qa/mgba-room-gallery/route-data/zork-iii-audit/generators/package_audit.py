from pathlib import Path
import csv,json,re
from PIL import Image,ImageDraw,ImageFont

root=Path.cwd();out=root/'build/zork-iii-audit'
font=ImageFont.truetype('/System/Library/Fonts/Menlo.ttc',14)
head=ImageFont.truetype('/System/Library/Fonts/Menlo.ttc',22)
bg=(14,18,24);paper=(230,227,208);dim=(168,182,188)
group={}
def assign(ids,issue):
    for i in ids:group.setdefault(i,[]).append(issue)
assign([12,140,184,216],'Z3-01');assign([124,192],'Z3-02');assign([75,93,163,182,203],'Z3-03')
assign([6,8,48,51,54,58,100,113,119,151,160,161,169,178,221,224,44,98],'Z3-04')
assign([39,143,192,103,179,16,15],'Z3-05');assign([176,66,103,179,16,28,227],'Z3-06')
assign([19,36,43,45,159,162,190,204,225],'Z3-07');assign([46],'Z3-08');assign([172],'Z3-09')
assign([211,55,64,15,39,40,81,86,116,136,143,188],'Z3-10');assign([176,194],'Z3-11');assign([218],'Z3-12');assign([194],'Z3-13')
logs=['connected-visual-refresh.log','probe-expanded.log','route-input.log','alternate-input.log','guardians-input.log','probe-input.log','probe-level-3.log','optional-eras.log','time-years.log','vista-destinations.log','persistence.log','crowded-persistence.log','guardians-failures.log','sailor-success.log','lake-failures.log','cliff-branches.log','sailor-branches.log','hood-branches.log','museum-failures.log','royal-branches.log','old-man-branches.log','beam-mirror-failures.log','master-prison-branches.log','light-outcomes.log','lamp-exhaustion.log','past-look.log','drop-stress.log','encounter-markers.log']
states=[];unique={};keys=set();ui_rooms=set()
for log in logs:
    p=out/log
    if not p.exists():continue
    for n,r,art,title,d in re.findall(r'^SHOT (\S+) room=(\d+) scene=(\S+) title=(.*?) dark=(\d)',p.read_text(),re.M):
        room=int(r);dark=int(d)
        if n.startswith('route-') and not dark:ui_rooms.add(room)
        if (n,room,dark) in keys:continue
        keys.add((n,room,dark));s={'name':n,'room':room,'art':art,'title':title,'dark':dark,'log':log}
        states.append(s)
        if not n.startswith('drops-'):unique.setdefault((room,dark),s)
catalog={int(i):n for i,parent,n in [l.split('\t') for l in (out/'objects.tsv').read_text().splitlines()] if parent=='34'}
captured={s['room'] for s in states};unchecked={i:n for i,n in catalog.items() if i not in captured}
renames={'vista-IV':'Vista IV: fatal altar narrative and revival','vista-IV-return':'After altar revival','cliff-down-refusal':'Down route to Cliff Base','cliff-jump-outcome':'Jump at Cliff Base refusal','time-wrong-year':'Machine already removed: control refusals','past-timeout':'Waiting after guards depart','sailor-shown':'Missed sailor window','sailor-gift':'No gift after missed window','vial-used':'No vial available after missed window'}
fields=['chapter','room_id','screen_state','title','background','dark','story_visual','placement','ui_access','persistence','followups','evidence']
with (root/'zork-iii-audit-screens.csv').open('w',newline='') as f:
    w=csv.DictWriter(f,fieldnames=fields);w.writeheader()
    for s in sorted(states,key=lambda s:(s['room'],s['dark'],s['name'])):
        r=s['room'];issues=group.get(r,[]);n=s['name']
        visual='Issue (room/state family; see findings)' if issues and not s['dark'] else 'Pass (sampled)'
        if r==194 and not n.startswith('drops-') and n not in ['puzzle-restored-stale','puzzle-continued','royal-book']:visual='Pass (diagram layout)'
        if r in [176,227] and n.startswith('route-'):visual='Pass (baseline scenery/vehicle); see state followups'
        if r==172 and n not in ['engravings-open','engravings-return']:visual='Pass (closed wall/actor baseline); see door followup'
        placement='Issue' if r in [12,124,218] or n.startswith('drops-') and r==176 else 'Unchecked (state-specific)'
        if r==194:placement='Issue: floor contents omitted' if n.startswith('drops-') or n=='royal-book' else 'N/A (diagram view; see floor-content followup)'
        if r in [30,109,181,217] and not s['dark']:placement='Pass (sampled)'
        ui='Pass (baseline menus sampled)' if r in ui_rooms else 'Unchecked'
        if r==218 and 'man' in n:ui='Issue: marker crosses header by 1 pixel'
        persist='Unchecked (state-specific)'
        if n in ['lamp-return','lamp-restored','lamp-continued','machine-restored','ring-hidden-restored','engravings-restored','bronze-restored-open','crowded-return','crowded-restored']:persist='Pass (targeted)'
        if n in ['puzzle-restored-stale','puzzle-continued']:persist='Issue: logical state restored; diagram requires B survey'
        evidence='build/zork-iii-audit/'+n+'.ppm'
        w.writerow(dict(chapter='III',room_id=r,screen_state=renames.get(n,n),title=s['title'],background=s['art'],dark=s['dark'],story_visual=visual,placement=placement,ui_access=ui,persistence=persist,followups='; '.join(issues),evidence=evidence))
    for r,title in sorted(unchecked.items()):
        w.writerow(dict(chapter='III',room_id=r,screen_state='Uncaptured catalog room',title=title,story_visual='Unchecked',placement='Unchecked',ui_access='Unchecked',persistence='Unchecked',evidence='Narration observed for 210 and 57; no stable interactive capture' if r in [210,57] else ''))

rows=sorted(unique.values(),key=lambda s:s['room'])
columns=4;cw=480;ch=202;top=88
board=Image.new('RGB',(columns*cw,top+((len(rows)+columns-1)//columns)*ch),bg);d=ImageDraw.Draw(board)
d.text((16,14),'Zork III — visual audit room inventory',font=head,fill=paper)
d.text((16,51),f'{len(rows)} sampled room/light states in {len(captured)} room IDs | production renderer | labels identify reused art',font=font,fill=dim)
for i,s in enumerate(rows):
    x=i%columns*cw;y=top+i//columns*ch
    label=f"{s['room']} {s['title']}"+(' [dark]' if s['dark'] else '')
    d.text((x+8,y+6),label,font=font,fill=paper);d.text((x+8,y+26),s['art'],font=font,fill=dim)
    im=Image.open(out/(s['name']+'.ppm')).crop((0,15,240,91)).resize((480,152),Image.Resampling.NEAREST);board.paste(im,(x,y+48))
board.save(root/'zork-iii-scene-review.png')

native=[('Underwater: above-water backdrop','underwater-object'),('Past 776: later cage still shown','past-jewel-description'),('Year picker: working controls','year-picker'),('Royal Puzzle: saved layout','puzzle-before-save'),('Royal Puzzle: Restore clears view','puzzle-restored-needs-survey'),('Royal Puzzle: B restores the diagram','puzzle-resurveyed'),('Crowded staff: selected, unseen','crowded-staff-missing'),('Same unseen staff: Take works','crowded-staff-taken'),('Beam blocked: no beam cue','beam-blocked'),('Cliff: distant marker reaches header','cliff-looking-down'),('Cliff rescue: grounded man and chest','cliff-reward'),('Hood removed: cloak remains','hood-mercy-cloak'),('Sailor: greeting target visible','sailor-visible'),('Sailor: vial lands on the sand','sailor-vial-gift'),('Guardians: close passage','guardians-near'),('Master: remote dial order','master-dial-picker'),('Prison: rotated state','cell-six'),('Treasury: ending still uses hall','ending-last-page')]
cw=480;ch=360;columns=3;top=88
board=Image.new('RGB',(cw*columns,top+((len(native)+columns-1)//columns)*ch),bg);d=ImageDraw.Draw(board)
d.text((16,14),'Zork III — audit findings and verified outcomes',font=head,fill=paper)
d.text((16,51),'Native 240 x 160 mGBA frames, enlarged 2x | audit findings are documented in zork-iii-audit.md',font=font,fill=dim)
for i,(label,n) in enumerate(native):
    x=i%columns*cw;y=top+i//columns*ch;d.text((x+8,y+10),label,font=font,fill=paper)
    f=out/('mgba-'+n+'.png');assert f.exists(),f
    board.paste(Image.open(f).resize((480,320),Image.Resampling.NEAREST),(x,y+36))
board.save(root/'zork-iii-audit-preview.png')
(out/'coverage.json').write_text(json.dumps({'captured_room_ids':sorted(captured),'room_light_states':len(unique),'state_checklist_rows':len(states),'unchecked_room_ids':unchecked,'scene_board_panels':rows,'native_preview_frames':[n for _,n in native]},indent=2)+'\n')
print(len(captured),'room IDs;',len(unique),'room/light panels;',len(states),'state checklist rows;',len(unchecked),'uncaptured catalog IDs')
