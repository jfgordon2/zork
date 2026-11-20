from pathlib import Path
import json,re
root=Path.cwd();assets=root/'assets';records=root/'build/zork-ii-finish/records'
preserved=[]
for f in sorted(records.glob('*.json')):
    d=json.loads(f.read_text())
    d['generated_source']=Path(d.pop('raw')).name
    d['generator']='Built-in ImageGen'
    d['review_document']='../zork-ii-completion-audit.md'
    preserved.append(d)
(assets/'zork-ii-completion-prompts.json').write_text(json.dumps({'generator':'Built-in ImageGen','rules':'SCENERY_RULES.md','records':preserved},indent=2)+'\n')
byname={d['name']:d for d in preserved}
p=assets/'zork-ii-scenes.json';data=json.loads(p.read_text());old={d['name']:d for d in data['backgrounds']}
rows=re.findall(r'ZORK2_SCENE\(([^,]+), (\d+), (\d+), (\d+), (\d+), (\d+)\)',(root/'src/zork2_scenes.def').read_text())
for name,room,x,y,x2,y2 in rows:
    if name not in byname:continue
    rec=byname[name]
    d=old.get(name,{})
    d.update(name=name,rooms=[int(room)] if int(room) else [],asset='z2_'+name+'.png',source_size=[240,102],visible_source_rows=[10,85],safe_contacts=[[int(x),int(y)],[int(x2),int(y2)]],review_document='../zork-ii-completion-audit.md',prompt=rec['prompt'],prompt_status='Preserved final generation or edit prompt',generated_source=rec['generated_source'],references=rec.get('refs',[]))
    if 'desc' in rec:d['story_and_composition']=rec['desc']
    if name.startswith('quarters_'):d['condition']='Room 69: actual most recently printed '+name.removeprefix('quarters_')+' description; cached in Save/Restore'
    if name=='workshop_open':d['condition']='Door 43 OPEN from Workshop 45: right jamb, swings inward, concealed hinge; actual Guarded Room through-view'
    if name=='guarded_open':d['condition']='Door 43 OPEN from Guarded Room 28: left jamb, swings away; same oak door; actual Workshop through-view'
    if name not in old:data['backgrounds'].append(d)
assert len(data['backgrounds'])==81
p.write_text(json.dumps(data,indent=2)+'\n')
p=assets/'zork-ii-item-prompts.json';data=json.loads(p.read_text());rec=byname['bucket_pov'];d={'name':'bucket_pov','asset':'items/bucket_pov.png','prompt':rec['prompt'],'reference':rec['ref'],'generated_source':rec['generated_source'],'native_bounds':[176,24]}
data['items']=[v for v in data['items'] if v['name']!='bucket_pov']+[d]
p.write_text(json.dumps(data,indent=2)+'\n')
