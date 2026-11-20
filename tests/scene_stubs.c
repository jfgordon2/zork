#include "chapter.h"
#include "video.h"

// Full-size zeroed backgrounds: scene code samples background pixels (for example the
// jewel cage bars), so a 1-pixel stub would be read out of bounds.
#define STUB_ART (SCREEN_W * SCENE_H)

// Host tests use the story VM and scene identifiers without embedding the GBA artwork ROM.
const uint8_t story_zork1[1], story_zork1_end[1], story_zork2[1], story_zork2_end[1],
              story_zork3[1], story_zork3_end[1];
const Color art_west[STUB_ART], art_north[STUB_ART], art_south[STUB_ART], art_rear[STUB_ART], art_kitchen[STUB_ART], art_living[STUB_ART],
            art_living_moved[STUB_ART], art_cellar[STUB_ART], art_barrow[STUB_ART], art_stairway[STUB_ART], art_cavern[STUB_ART], art_water[STUB_ART],
            art_hall[STUB_ART], art_forest[STUB_ART], art_troll_room[STUB_ART], art_riddle_room[STUB_ART], art_riddle_room_open[STUB_ART],
            art_z2_riddle_room_238[STUB_ART], art_z2_riddle_room_238_open[STUB_ART],
            art_mirror_room_v2[STUB_ART], art_canyon[STUB_ART], art_dam[STUB_ART], art_egyptian[STUB_ART],
            art_carousel[STUB_ART], art_tea[STUB_ART], art_lake_shore[STUB_ART], art_jewel_room[STUB_ART], art_treasure_room[STUB_ART],
            art_atlantis[STUB_ART], art_great_cavern[STUB_ART], art_workroom[STUB_ART], art_cliff_ledge_upper_shelf[STUB_ART], art_flathead_ocean[STUB_ART],
            art_cyclops_room[STUB_ART], art_aragain_falls[STUB_ART], art_end_of_rainbow[STUB_ART], art_pentagram_room[STUB_ART], art_menhir_room[STUB_ART],
            art_flathead_ship[STUB_ART], art_dragon_room[STUB_ART], art_robot_machine[STUB_ART], art_land_of_shadow[STUB_ART],
            art_dungeon_entrance[STUB_ART], art_aquarium[STUB_ART], art_aquarium_broken[STUB_ART],
            art_bank_vault[STUB_ART], art_cerberus_room[STUB_ART], art_hades[STUB_ART], art_engravings[STUB_ART], art_cliff_top[STUB_ART],
            art_bank_small_room[STUB_ART], art_bank_depository[STUB_ART], art_crypt_anteroom[STUB_ART], art_crypt_anteroom_open[STUB_ART],
            art_hades_inside[STUB_ART], art_cliff_base[STUB_ART], art_junction[STUB_ART], art_technology_museum[STUB_ART], art_museum_entrance[STUB_ART],
            art_museum_entrance_open[STUB_ART];

#define ZORK1_SCENE(name) const Color art_##name[STUB_ART];
#include "zork1_scenes.def"
#undef ZORK1_SCENE

#define ZORK2_SCENE(name, room, lx, ly, rx, ry) const Color art_z2_##name[1];
#include "zork2_scenes.def"
#undef ZORK2_SCENE
