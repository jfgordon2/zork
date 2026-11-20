#include "video.h"

// Full-size zeroed backgrounds: scene code samples background pixels (for example the
// jewel cage bars), so a 1-pixel stub would be read out of bounds.
#define STUB_ART (SCREEN_W * SCENE_H)

// story_test exercises the Zork III room mapper without linking the GBA scene ROM.
const Color art_z3_underwater[STUB_ART], art_z3_lake_surface[STUB_ART], art_z3_west_shore[STUB_ART], art_z3_south_shore[STUB_ART];
const Color art_z3_aqueduct[STUB_ART], art_z3_high_arch[STUB_ART], art_z3_water_slide[STUB_ART], art_z3_damp_passage[STUB_ART];
const Color art_z3_aqueduct_view[STUB_ART], art_z3_scenic_vista[STUB_ART];
const Color art_z3_technology_unfinished[STUB_ART], art_z3_jewel_unfinished[STUB_ART], art_z3_treasury[STUB_ART];
const Color art_z3_crystal_grotto[STUB_ART], art_z3_foggy_room[STUB_ART], art_z3_great_door[STUB_ART];
const Color art_z3_endless_stair[STUB_ART], art_z3_marble_hall[STUB_ART], art_z3_marble_hall_prison_open[STUB_ART];
const Color art_z3_prison_cell_open[STUB_ART];
const Color art_z3_prison_cell_closed[STUB_ART], art_z3_beam_room[STUB_ART], art_z3_button_room[STUB_ART];
const Color art_z3_timber_room[STUB_ART], art_z3_royal_puzzle_entrance[STUB_ART], art_z3_royal_hall[STUB_ART];
const Color art_z3_mirror_hall_closed[STUB_ART];
const Color art_z3_narrow_mirror[STUB_ART], art_z3_parapet[STUB_ART];
const Color art_z3_mirror_hall_open[STUB_ART], art_z3_guardian_hallway_v3[STUB_ART];
const Color art_z3_ladder_top[STUB_ART], art_z3_ladder_bottom[STUB_ART], art_z3_mine_dead_end[STUB_ART], art_z3_side_room[STUB_ART];
const Color art_z3_narrow_mirror_guardians[STUB_ART], art_z3_narrow_mirror_guardians_north[STUB_ART];
const Color art_z3_narrow_mirror_west[STUB_ART], art_z3_narrow_mirror_guardians_west[STUB_ART];
const Color art_z3_narrow_mirror_guardians_north_west[STUB_ART];
const Color art_z3_barren_area[STUB_ART], art_z3_dead_end[STUB_ART], art_z3_east_corridor[STUB_ART];
const Color art_z3_hairpin_loop[STUB_ART], art_z3_key_room[STUB_ART];
const Color art_z3_narrow_corridor[STUB_ART], art_z3_puzzle_room[STUB_ART], art_z3_south_corridor[STUB_ART];
const Color art_z3_tight_squeeze[STUB_ART], art_z3_west_corridor[STUB_ART];
const Color art_z3_creepy_crawl[STUB_ART], art_z3_prison_cell_bronze[STUB_ART], art_z3_shadow_cliff[STUB_ART];
