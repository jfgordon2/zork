#ifndef ZORK_ROOM_VISUALS_H
#define ZORK_ROOM_VISUALS_H

#include "chapter.h"
#include "puzzle_map.h"

enum { STORY_ICON_SIZE = 9 };

void draw_story_icon(unsigned chapter, unsigned object, int x, int y);
void draw_room_details(unsigned chapter, unsigned room, const Color *background,
                       const Targets *targets, unsigned focused);
void draw_target_focus_box(bool visible);
void draw_z3_ending_cue(unsigned stage);
void draw_puzzle_map(const PuzzleMap *map, const Targets *targets, unsigned focused);
bool room_visuals_animate(unsigned chapter, unsigned room, unsigned story_stage);

#endif
