#include "chapter.h"

// The replay harness loads stories from disk; the game object still needs
// placeholder story symbols because trilogy.c is linked into the host binary.
const uint8_t story_zork1[1], story_zork1_end[1], story_zork2[1], story_zork2_end[1],
              story_zork3[1], story_zork3_end[1];
