from pathlib import Path
import re,subprocess

root = Path('build/zork-i-mine-audit')
def restore(name):
    return f'@{root}/{name}.sav'
def save(name, focus=0):
    return [f'|{name} {focus}', f'!{root}/{name}.sav']
def run(name, commands, seed=7):
    source=root/f'{name}.txt'
    source.write_text('\n'.join(commands)+'\n')
    with (root/f'{name}.log').open('w') as dest:
        subprocess.run([str(root/'audit'),str(seed),str(source)],stdout=dest,check=True)

run('opening-negative', ['open door','take leaflet','open mailbox',*save('mailbox-open-with-leaflet',230),
                         'read leaflet','take leaflet',*save('mailbox-empty-open',230),
                         'close mailbox',*save('mailbox-closed',230),
                         'south','east','enter window',*save('window-jammed',243),
                         'open window',*save('window-open',243),'enter window','west'])
run('darkness', [restore('hatch-open'),'turn off lamp','down',*save('cellar-dark'),
                 'north','s','e','n',*save('grue-end'),
                 restore('hatch-open'),'down','north',*save('troll-safe',150)])
run('grating-top', ['w','n','look',*save('grating-buried',29),'move leaves',*save('grating-exposed',29),
                    'open grating','down','look'])
grate = Path('build/enc-grate-below.txt').read_text().splitlines()
index=grate.index('unlock grating with key')
run('grating-below',[*grate[:index],*save('grating-locked-below',29),'open grating','up',
                     restore('grating-locked-below'),*grate[index:index+2],*save('grating-open-below',29),
                     'up',*save('grating-open-above',29),'down',*save('grating-return',29)])
food = re.findall(r'^>>>> (.*?) \[room #',Path('build/z1-alt-cyclops.log').read_text(),re.M)
index=food.index('give lunch to cyclops')
run('cyclops-food', [*food[:index],*save('cyclops-food-start',199),'give lunch to cyclops',
                     *save('cyclops-thirsty',199),'give bottle to cyclops',
                     *save('cyclops-closed-bottle',199),'open bottle','give bottle to cyclops',
                     *save('cyclops-sleep',199),'up',*save('thief-after-cyclops',164),
                     restore('cyclops-food-start'),'up','wait','wait','wait',*save('cyclops-death')],seed=7)
run('troll-actions',[restore('troll-armed'),'take axe','go north','attack troll',
                    restore('troll-armed'),'kill troll with sword','kill troll with sword','kill troll with sword',
                    *save('troll-defeated',36)])
run('hades-actions',[restore('hades-warded'),'south','read book','south',
                     restore('hades-warded'),'ring bell','south',
                     restore('hades-cleared'),'south',*save('skull-focused',140),'take skull'])
run('dam-actions',[restore('dam-before'),'turn bolt with knife',*save('dam-wrong-tool',210),
                    restore('dam-before'),'turn bolt with wrench','look',*save('dam-sluice-open',210),
                    'turn bolt with wrench','look',*save('dam-sluice-closed',210),
                    restore('route-224-lit'),'press blue button','look',*save('maintenance-flooding',237),
                    *(['wait']*16),*save('maintenance-drowned')])
run('river-actions',[restore('route-111-lit'),'launch','swim',*save('swim-response'),
                      restore('boat-before-launch'),'launch',*save('river-launched',208),
                      restore('route-182-lit'),'wait','wait','wait','wait',*save('river-over-falls'),
                      restore('route-182-lit'),'east','get out of boat',*save('boat-beached',208)])
run('gas-actions',[restore('route-46-lit'),'look',*save('gas-lantern',214),
                    'up','south','take torch','north','down',*save('gas-with-flame')])
run('rainbow-actions',[restore('rainbow-immaterial'),'cross rainbow',
                        restore('rainbow-solid'),'cross rainbow',*save('rainbow-crossed',59),'cross rainbow',
                        *save('rainbow-return',59)])
run('tree-actions',[restore('route-5-lit'),*save('egg-in-nest',105),'take egg',
                    'open egg','drop egg',*save('egg-dropped',105)])
run('other-food',[restore('route-27-lit'),'open sack',*save('garlic-on-table'),
                  'take garlic',*save('garlic-carried',99)])

run('visual-branches', [restore('route-4-lit'),*save('trident-selected',23),
    restore('coffin-closed'),*save('egypt-closed',179),
    restore('coffin-open'),*save('egypt-open',179), 'take sceptre',*save('egypt-empty',179),
    restore('route-11-lit'),'tie rope to railing',*save('dome-tied',58),
    restore('route-245-lit'),*save('shaft-up',108),'lower basket',*save('shaft-down',203),
    restore('route-249-lit'),*save('shaft-bottom',108),
    restore('route-129-lit'),*save('ritual-altar',196),
    restore('bell-rung'),*save('hot-bell',168),
    restore('route-165-lit') if (root/'route-165-lit.sav').exists() else restore('route-187-lit'),
    *save('attic-rope',58),
    restore('route-218-lit'),*save('skeleton-key',96),
    restore('cyclops-ran'),'east',*save('strange-west'),'east',*save('shortcut-from-house',100),
    'west',*save('strange-east'),
    restore('route-110-lit'),*save('mirror-south',234),'rub mirror',*save('mirror-north',183)])
run('drop-checks', [restore('route-27-lit'),*save('bottle-table',138),'take bottle',
    *save('bottle-carried'), 'drop bottle', *save('bottle-floor',138),
    restore('route-75-lit'),'take lamp','turn on lamp','take sword','drop lamp','drop sword',
    *save('living-dropped',146),
    restore('route-18-lit'),'take torch','drop torch',*save('torch-floor',12),
    restore('egypt-open'), 'drop bell','drop pump','drop match','drop lamp',
    'take coffin',*save('coffin-carried'), 'drop coffin',*save('coffin-floor',179),
    restore('coffin-floor'),*save('coffin-restored',179),
    restore('route-37-lit'), 'drop lamp',*save('boat-with-lantern',146),
    'wait',*save('cargo-downstream',146),
    restore('route-171-lit'),'drop garlic','north',*save('bat-carryaway'),
    restore('route-171-lit'),'north',*save('bat-safe',70)])
