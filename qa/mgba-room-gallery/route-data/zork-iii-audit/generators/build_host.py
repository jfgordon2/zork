from pathlib import Path
import subprocess
root=Path.cwd();out=root/'build/zork-iii-audit'
common=['cc','-O2','-std=c11','-fno-builtin','-Isrc','-Ibuild','-I'+str(out)]
sources=['src/zmachine.c','src/save.c','src/chapter.c','src/interaction.c','src/puzzle_map.c','src/trail.c','src/room_map.c','src/visual_state.c']
subprocess.run(common+['-DVISUAL_QA',str(out/'audit.c'),'src/room_visuals.c',*sources,str(out/'art.S'),'-o',str(out/'audit')],check=True)
subprocess.run(common+[str(out/'controller-replay.c'),*sources,'src/video.c',str(out/'art.S'),'-o',str(out/'controller-replay')],check=True)
