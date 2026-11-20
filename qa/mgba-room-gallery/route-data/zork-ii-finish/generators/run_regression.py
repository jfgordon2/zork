from pathlib import Path
import subprocess,re,json
root=Path.cwd();out=root/'build/zork-ii-finish/re-audit';old=root/'build/zork-ii-art/re-audit'
def run(name,source):
    with (out/(name+'.log')).open('w') as f: subprocess.run([str(out/'audit'),'4372',str(source)],stdout=f,check=True)
    print(name+': '+(out/(name+'.log')).read_text().splitlines()[-1],flush=True)
def story(log):
    return '\n'.join(line for line in log.splitlines() if not line.startswith(('SHOT ','ACCESS ','PICKERS ','AUDIT RESULT ','FINISH ')) and line.strip())
run('route',root/'build/zork-ii-audit/route-input.txt')
cases=['fixtures-input','riddle-failures','bucket-routes','cake-outcomes','robot-failures','keyhole-failures','door-check','dragon-failures','sphere-branches','balloon-failures','bank-routes','demon-failures','crypt-states','branch-extra','branch-final','volcano-gnome']
checks={}
for name in cases:
    run(name,old/(name+'.txt'))
    checks[name]=story((out/(name+'.log')).read_text())==story((old/(name+'.log')).read_text())
    assert checks[name],name+' transcript changed'
checks['route']=story((out/'route.log').read_text())==story((old/'route.log').read_text())
assert checks['route']
run('persistence',root/'build/zork-ii-audit/persistence-cases.txt')
run('extras',old/'extras.txt')
run('targeted',old/'targeted.txt')
with (out/'controller.log').open('w') as f: subprocess.run([str(out/'controller-replay'),'2',str(out/'route.log'),'strict'],stdout=f,check=True)
print((out/'controller.log').read_text().splitlines()[-2:],flush=True)
(out/'regression-results.json').write_text(json.dumps({'story_transcripts_match':checks},indent=2)+'\n')
print('All regression checks passed.',flush=True)
