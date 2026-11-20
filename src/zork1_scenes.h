#ifndef ZORK1_SCENES_H
#define ZORK1_SCENES_H

#include "video.h"

#define ZORK1_SCENE(name) extern const Color art_##name[];
#include "zork1_scenes.def"
#undef ZORK1_SCENE

#endif
