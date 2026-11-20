#ifndef ZORK2_SCENES_H
#define ZORK2_SCENES_H

#include "video.h"

#define ZORK2_SCENE(name, room, lx, ly, rx, ry) extern const Color art_z2_##name[];
#include "zork2_scenes.def"
#undef ZORK2_SCENE

#endif
