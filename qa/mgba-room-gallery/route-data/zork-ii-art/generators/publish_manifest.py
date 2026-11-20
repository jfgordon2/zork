from pathlib import Path
import json,re
root=Path.cwd();assets=root/'assets';scratch=root/'build/zork-ii-art'
old=json.loads((assets/'zork-ii-scenes.json').read_text());jobs={x['name']:x for x in old['backgrounds']}
records={x['name']:x for p in (scratch/'records').glob('*.json') if (x:=json.loads(p.read_text())).get('name')}
for x in json.loads((scratch/'generated.json').read_text()):records.setdefault(x['name'],x)
# Retain the final corrective edit rather than the rejected basket-bearing candidate.
records['narrow_ledge']={'prompt':(scratch/'narrow_ledge-clean-plate-prompt.txt').read_text(),'raw':'exec-afb38cb8-b4d3-4cb5-a64f-ffe9bd1a3452.png'}
records['pool']={'prompt':'Edit the dry-basin view: fill its depression with shallow blue-green salt water. Preserve the ceiling leak, western exit, camera, walls and floor. No window, candy, cake, flask or props.','raw':'exec-b876ff7c-9f84-4640-b9c4.png','prompt_status':'Reconstructed edit recipe; native PNG is authoritative.'}
conditions={'pool_dry':'Pool39 no longer in room82','ice_melted':'Glacier111 no longer in room161','marble_open':'MARBLE-DOOR global163 set','tiny_open':'Door137 OPEN from TinyRoom46','dreary_open':'Door137 OPEN from DrearyRoom59','guarded_open':'Door43 OPEN from GuardedRoom28; opens away from viewer','safe_blast':'Debris101 in DustyRoom182','crypt_dark':'Crypt125 has no accessible lit matchbook, lamp or newspaper; secret151 shut','crypt_open':'Crypt125 illuminated and secret151 OPEN','workshop_open':'Door43 OPEN from Workshop45; preserve user-approved earlier view','crypt_secret_open':'Crypt125 unilluminated and secret151 OPEN','angled_entry_open':'Odd-room secret global13 set','narrow_ledge_escape':'Volcano escape global72 set'}
entries=[]
for name,room,lx,ly,rx,ry in re.findall(r'^ZORK2_SCENE\((\w+), (\d+), (\d+), (\d+), (\d+), (\d+)\)',(root/'src/zork2_scenes.def').read_text(),re.M):
 record=records.get(name,{})
 entry={'name':name,'rooms':[int(room)] if int(room) else [],'asset':f'z2_{name}.png','source_size':[240,102],'visible_source_rows':[10,85],'safe_contacts':[[int(lx),int(ly)],[int(rx),int(ry)]],'review_document':'../zork-ii-re-audit.md'}
 if name in conditions:entry['condition']=conditions[name]
 source=jobs.get(name,{}).get('story_description')
 if source:entry['story_description']=source
 prompt=record.get('prompt')
 geometry=scratch/f'{name}-geometry-prompt.txt'
 if geometry.exists():prompt=geometry.read_text()
 if prompt:entry['prompt']=prompt;entry['prompt_status']=record.get('prompt_status','Preserved generation or final edit prompt')
 else:entry['prompt_status']='Exact original prompt unavailable; regenerate from bundled story and SCENERY_RULES.md, using this final PNG as the layout reference.'
 if record.get('raw'):entry['generated_source']=Path(record['raw']).name
 entries.append(entry)
# Do not carry forward inaccurate draft composition notes as future instructions.
(assets/'zork-ii-scenes.json').write_text(json.dumps({'generator':'Built-in ImageGen','rules':'SCENERY_RULES.md','generation_policy':old['generation_policy'],'backgrounds':entries},indent=2)+'\n')
items=[]
new_names={x['name'] for x in json.loads((scratch/'item-jobs.json').read_text())}|{'bucket','rusty_key','place_mat','letter_opener'}
for name in sorted(new_names):
 record=records.get(name,{})
 if not (assets/'items'/f'{name}.png').exists():continue
 entry={'name':name,'asset':f'items/{name}.png'}
 if record.get('prompt'):entry['prompt']=record['prompt']
 if record.get('raw'):entry['generated_source']=Path(record['raw']).name
 if name=='dead_serpent':entry['identity_reference']='encounter-options/imagegen-sprites/serpent.png'
 items.append(entry)
(assets/'zork-ii-item-prompts.json').write_text(json.dumps({'generator':'Built-in ImageGen','rules':'SCENERY_RULES.md','items':items},indent=2)+'\n')
print(f'{len(entries)} backgrounds; {len(items)} item poses')
