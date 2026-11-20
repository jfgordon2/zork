#ifndef ZORK_CHAPTER_H
#define ZORK_CHAPTER_H

#include "video.h"
#include "zmachine.h"

enum {
    TARGET_LIMIT = 96, ROYAL_PUZZLE_ROOM = 194,
    ZORK1_LADDER_TOP = 16, ZORK1_GAS_ROOM = 46, ZORK1_LADDER_BOTTOM = 72,
    ZORK1_MINE_LADDER = 88, ZORK1_TROLL = 150, ZORK1_STAIRS = 177, ZORK1_BRACELET = 214,
    ZORK2_DRAGON = 192, ZORK2_DRAGON_ROOM = 248,
    ZORK2_DRAGONS_LAIR = 201, ZORK2_AQUARIUM_ROOM = 233, ZORK2_AQUARIUM = 250, ZORK2_SERPENT = 32,
    ZORK2_BANK_SMALL_ROOM = 64, ZORK2_CRYPT_DOOR = 245,
    ZORK3_OLD_MAN = 97, ZORK3_HOODED_FIGURE = 110, ZORK3_CLIFF_MAN = 189, ZORK3_CLIFF_LEDGE = 218,
    ZORK3_DUNGEON_ENTRANCE = 105, ZORK3_DUNGEON_DOOR = 207,
    ZORK3_GUARDIANS_HALL = 44, ZORK3_GUARDIANS_SOUTH = 54, ZORK3_GUARDIANS_NORTH = 224,
    ZORK3_VIKING_SHIP = 214, ZORK3_SHIP_HIDDEN = 10,
    ZORK3_BEAM_ROOM = 6, ZORK3_UNDERWATER = 12, ZORK3_NORTH_CORRIDOR = 19,
    ZORK3_TIMBER_ROOM = 15,
    ZORK3_MUSEUM_ENTRANCE = 28, ZORK3_PARAPET = 36, ZORK3_GREAT_DOOR = 136,
    ZORK3_FOGGY_ROOM = 64, ZORK3_TECHNOLOGY_PAST = 66, ZORK3_AQUEDUCT_VIEW = 75,
    ZORK3_CRYSTAL_GROTTO = 81, ZORK3_PUZZLE_ENTRANCE = 84, ZORK3_BUTTON_ROOM = 98,
    ZORK3_MUSEUM_ENTRANCE_ALT = 173, ZORK3_JEWEL_947 = 16, ZORK3_JEWEL_PRESENT = 103,
    ZORK3_JEWEL_PAST = 179, ZORK3_LAKE = 184,
    ZORK3_ROOM_8 = 192, ZORK3_AQUEDUCT = 203, ZORK3_PRISON_CELL_ALT = 204,
    ZORK3_ENDLESS_STAIR = 211, ZORK3_SOUTH_SHORE = 216, ZORK3_HIGH_ARCH = 182,
    ZORK3_WEST_SHORE = 140, ZORK3_DAMP_PASSAGE = 93, ZORK3_WATER_SLIDE = 163,
    ZORK3_SCENIC_VISTA = 124, ZORK3_ROYAL_HALL = 116, ZORK3_TREASURY = 46,
    ZORK3_ENGRAVINGS_ROOM = 172,
    ZORK3_LADDER_TOP = 20, ZORK3_LADDER_BOTTOM = 57, ZORK3_MINE_DEAD_END = 138,
    ZORK3_SIDE_ROOM = 130, ZORK3_SIDE_ROOM_ALT = 188,
    ZORK3_NARROW_GUARDIANS_EAST = 48, ZORK3_NARROW_GUARDIANS_WEST = 151,
    ZORK3_NARROW_STATUES_EAST = 58, ZORK3_NARROW_STATUES_WEST = 161
};

typedef struct {
    const char *title, *subtitle;
    const uint8_t *story, *end;
    unsigned player, local_prop;
    const Color *opening;
} Chapter;

typedef struct { uint16_t id[TARGET_LIMIT]; unsigned count; } Targets;

bool zork2_fuse_burning(void);

enum {
    TARGET_SELF = 256, TARGET_NORTH_WALL, TARGET_EAST_WALL, TARGET_SOUTH_WALL,
    TARGET_WEST_WALL
};

enum {
    ATTR_INVISIBLE = 7, ATTR_OPEN = 11, ATTR_TRANSPARENT = 12, ATTR_READABLE = 16,
    ATTR_PORTABLE = 17, ATTR_CONTAINER = 18, ATTR_LIT = 19, ATTR_EDIBLE = 20,
    ATTR_DOOR = 22, ATTR_CLIMBABLE = 23, ATTR_WEAPON = 29, ATTR_ACTOR = 30, ATTR_LIGHT = 31
};

extern const Chapter chapters[3];
extern const char *const path_names[12];
extern const char *const path_commands[12];
extern const uint8_t path_properties[12];
extern const Color art_west[], art_guardians[];

bool story_dark(unsigned chapter);
bool zork1_barrow_finished(void);
bool zork3_mirror_panel_open(void);
bool story_figure_defenseless(void);
bool story_cliff_man_visible(void);
const Color *story_scene(unsigned chapter, const char *room);
void story_targets(unsigned chapter, Targets *room, Targets *bag);
unsigned story_focus_dropped_item(const Targets *previous_bag, unsigned room, const Targets *current_room);
void story_name(unsigned chapter, unsigned object, char buffer[Z_NAME_MAX]);
void story_noun(unsigned object, char buffer[Z_NAME_MAX]);
bool text_has(const char *text, const char *part);
bool text_equal(const char *a, const char *b);
bool append_text(char *out, unsigned capacity, const char *text);

#endif
