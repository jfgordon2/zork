from pathlib import Path
import re,subprocess
root=Path.cwd();out=root/'build/zork-ii-finish/re-audit';out.mkdir(parents=True,exist_ok=True)
old=root/'build/zork-ii-audit'
# This harness runs the actual current cartridge code with memory-copy video on host.
s=(root/'src/trilogy.c').read_text().replace('#define SRAM ((volatile uint8_t *)0x0E000000)','#define SRAM audit_sram')
s=s.replace('#define SRAM audit_sram','static uint8_t audit_sram[SRAM_SIZE];\n#define SRAM audit_sram')
(out/'trilogy-host.inc').write_text(s)
s=(root/'src/video.c').read_text();a=s.index('    REG32(0x040000D4)');b=s.index('\n}',a)
s=s[:a]+'    memcpy(dst, src, words * 4);'+s[b:];(out/'video-host.inc').write_text(s)
for name in ['access.inc','persistence.inc','persist-cases.inc','controller-replay.c']:(out/name).write_text((old/name).read_text())
(out/'access.inc').write_text((out/'access.inc').read_text().replace('update(key, key);','update(key);'))
s=(old/'audit.c').read_text().replace('build/zork-ii-audit/%s','build/zork-ii-finish/re-audit/%s');s=s.replace('#include \"persistence.inc\"','#include \"persistence.inc\"\n#include \"../finish.inc\"')
s=s.replace('save_read(sram,chapter,visited,&trail)','save_read(sram,chapter,visited,&trail,&puzzle_map)')
s=s.replace('save_write(sram,chapter,visited,&trail)','save_write(sram,chapter,visited,&trail,&puzzle_map)')
s=s.replace('build/zork-ii-audit/%s.ppm','build/zork-ii-finish/re-audit/%s.ppm')
s=s.replace('build/zork-ii-audit/%s.sav','build/zork-ii-finish/re-audit/%s.sav')
s=s.replace('assert(argc==3);','assert(argc==3); setvbuf(stdout,NULL,_IONBF,0);')
s=s.replace("if (*call=='&')","if (*call=='^') {finish_case(call[1]);continue;}\n        if (*call=='&')")
s=s.replace('save_read(sram,chapter,visited,&trail)','save_read(sram,chapter,visited,&trail,&puzzle_map)')
s=s.replace('save_write(sram,chapter,visited,&trail)','save_write(sram,chapter,visited,&trail,&puzzle_map)')
s=s.replace('build/zork-ii-audit/%s.ppm','build/zork-ii-finish/re-audit/%s.ppm')
s=s.replace('build/zork-ii-audit/%s.sav','build/zork-ii-finish/re-audit/%s.sav')
s+='\nvoid visual_qa_ui_focus(void) {}\n'
(out/'audit.c').write_text(s)
names=re.findall(r'^SCENE (\w+)',(root/'src/scenes.S').read_text(),re.M)
names+=re.findall(r'^ZORK1_SCENE\(([^)]+)\)',(root/'src/zork1_scenes.def').read_text(),re.M)
names+=['z2_'+n for n in re.findall(r'^ZORK2_SCENE\(([^,]+),',(root/'src/zork2_scenes.def').read_text(),re.M)]
names=list(dict.fromkeys(names));decl=''.join(f'extern const Color art_{n}[];\n' for n in names)
(out/'art-dictionary.inc').write_text(decl+'static const struct {const char *name; const Color *art;} palette[] = {\n'+''.join(f'{{"{n}",art_{n}}},\n' for n in names)+'};\n')
(out/'art.S').write_text('.section __TEXT,__const\n.p2align 2\n'+''.join(f'.globl _art_{n}\n_art_{n}:\n.incbin "{root}/build/{n}.rgb555"\n' for n in names))
with (out/'art.S').open('a') as f:f.write('\n.globl _story_zork1'+(old/'art.S').read_text().split('.globl _story_zork1',1)[1])
common=['cc','-O2','-std=c11','-fno-builtin','-Isrc','-Ibuild','-I'+str(out)]
sources=['src/zmachine.c','src/save.c','src/chapter.c','src/interaction.c','src/puzzle_map.c','src/trail.c','src/room_map.c','src/visual_state.c','src/transcript_codec.c']
subprocess.run(common+['-DVISUAL_QA',str(out/'audit.c'),'src/room_visuals.c',*sources,str(out/'art.S'),'-o',str(out/'audit')],check=True)
subprocess.run(common+[str(out/'controller-replay.c'),*sources,'src/video.c',str(out/'art.S'),'-o',str(out/'controller-replay')],check=True)
print(out)
