#include "chapter.h"
#include "zork1_scenes.h"
#include "zork2_scenes.h"
#include "visual_state.h"

extern const uint8_t story_zork1[], story_zork1_end[], story_zork2[], story_zork2_end[], story_zork3[], story_zork3_end[];
extern const Color art_north[], art_south[], art_rear[], art_kitchen[], art_living[], art_living_moved[], art_cellar[];
extern const Color art_barrow[], art_stairway[], art_cavern[], art_water[], art_hall[], art_forest[];

extern const Color art_troll_room[], art_riddle_room[], art_riddle_room_open[], art_mirror_room_v2[];
extern const Color art_z2_riddle_room_238[], art_z2_riddle_room_238_open[];
extern const Color art_canyon[], art_dam[], art_egyptian[], art_carousel[], art_tea[];
extern const Color art_lake_shore[], art_jewel_room[];
extern const Color art_treasure_room[], art_atlantis[], art_great_cavern[], art_workroom[];
extern const Color art_cliff_ledge_upper_shelf[], art_flathead_ocean[];
extern const Color art_cyclops_room[], art_aragain_falls[], art_end_of_rainbow[], art_pentagram_room[];
extern const Color art_menhir_room[], art_flathead_ship[];
extern const Color art_dragon_room[], art_robot_machine[], art_land_of_shadow[], art_dungeon_entrance[];
extern const Color art_aquarium[], art_aquarium_broken[];
extern const Color art_bank_vault[], art_cerberus_room[], art_hades[], art_engravings[], art_cliff_top[];
extern const Color art_bank_small_room[], art_bank_depository[], art_crypt_anteroom[], art_crypt_anteroom_open[];
extern const Color art_hades_inside[], art_cliff_base[];
extern const Color art_junction[], art_technology_museum[], art_museum_entrance[], art_museum_entrance_open[];
extern const Color art_z3_underwater[], art_z3_lake_surface[], art_z3_west_shore[], art_z3_south_shore[];
extern const Color art_z3_aqueduct[], art_z3_high_arch[], art_z3_water_slide[], art_z3_damp_passage[];
extern const Color art_z3_aqueduct_view[], art_z3_scenic_vista[];
extern const Color art_z3_technology_unfinished[], art_z3_jewel_unfinished[], art_z3_treasury[];
extern const Color art_z3_crystal_grotto[], art_z3_foggy_room[], art_z3_great_door[];
extern const Color art_z3_endless_stair[], art_z3_marble_hall[], art_z3_marble_hall_prison_open[];
extern const Color art_z3_beam_room[], art_z3_button_room[], art_z3_timber_room[];
extern const Color art_z3_royal_puzzle_entrance[], art_z3_royal_hall[];
extern const Color art_z3_mirror_hall_closed[], art_z3_mirror_hall_open[];
extern const Color art_z3_narrow_mirror[], art_z3_parapet[];
extern const Color art_z3_narrow_mirror_guardians[], art_z3_narrow_mirror_guardians_north[];
extern const Color art_z3_narrow_mirror_west[], art_z3_narrow_mirror_guardians_west[];
extern const Color art_z3_narrow_mirror_guardians_north_west[];
extern const Color art_z3_prison_cell_open[], art_z3_prison_cell_closed[];
extern const Color art_z3_ladder_top[], art_z3_ladder_bottom[], art_z3_mine_dead_end[], art_z3_side_room[];
extern const Color art_z3_barren_area[], art_z3_dead_end[], art_z3_east_corridor[];
extern const Color art_z3_guardian_hallway_v3[], art_z3_hairpin_loop[], art_z3_key_room[];
extern const Color art_z3_narrow_corridor[], art_z3_puzzle_room[], art_z3_south_corridor[];
extern const Color art_z3_tight_squeeze[], art_z3_west_corridor[];
extern const Color art_z3_creepy_crawl[], art_z3_prison_cell_bronze[], art_z3_shadow_cliff[];
extern const Color art_z3_land_of_shadow_69[];

const Chapter chapters[3] = {
    {"Zork I", "The Great Underground Empire", story_zork1, story_zork1_end, 44, 5, art_west},
    {"Zork II", "The Wizard of Frobozz", story_zork2, story_zork2_end, 40, 12, art_barrow},
    {"Zork III", "The Dungeon Master", story_zork3, story_zork3_end, 35, 10, art_stairway},
};

const char *const path_names[12] = {"North", "East", "South", "West", "Northeast", "Southeast", "Southwest", "Northwest",
                                  "Up", "Down", "In", "Out"};
const char *const path_commands[12] = {"n", "e", "s", "w", "ne", "se", "sw", "nw", "up", "down", "in", "out"};
const uint8_t path_properties[12] = {31, 30, 28, 29, 27, 25, 24, 26, 23, 22, 21, 20};

static char lower(char ch) { return ch >= 'A' && ch <= 'Z' ? ch + ('a' - 'A') : ch; }

bool text_has(const char *text, const char *part) {
    for (; *text; text++) {
        unsigned i = 0;
        while (part[i] && text[i] && lower(part[i]) == lower(text[i])) i++;
        if (!part[i]) return true;
    }
    return !*part;
}

bool text_equal(const char *a, const char *b) {
    while (*a && lower(*a) == lower(*b)) { a++; b++; }
    return lower(*a) == lower(*b);
}

bool append_text(char *out, unsigned capacity, const char *text) {
    unsigned i = 0;
    while (i < capacity && out[i]) i++;
    while (i + 1 < capacity && *text) out[i++] = *text++;
    if (i < capacity) out[i] = 0;
    return !*text;
}

bool story_dark(unsigned chapter) {
    // LIT is at a different global index in each of the bundled releases.
    static const uint8_t globals[] = {51, 52, 62};
    return z_global(globals[chapter]) == 0;
}

bool zork3_mirror_panel_open(void) {
    // In the bundled Zork III story, global 107 tracks the Hallway 169
    // mirror panel. The panel is not a Z-machine OPEN object, so ATTR_OPEN
    // remains clear even while the story says it has swung outward.
    return z_global(107) != 0;
}

bool story_figure_defenseless(void) {
    return z_global(16) == 1; // S-STRENGTH in the bundled Zork III release.
}

bool story_cliff_man_visible(void) {
    return z_global(0) == ZORK3_CLIFF_LEDGE && z_global(79) != 0; // MAN-FLAG
}

bool zork1_barrow_finished(void) {
    // Release 119 enters a separate end-of-game prompt without changing HERE.
    if (z_global(0) != 244) return false;
    unsigned pc = z_resume().pc;
    return pc == 0x74ba || pc == 0x74f6;
}

static const Color *zork1_living_room(void) {
    bool shortcut = z_global(87) != 0;  // CYCLOPS-FLED
    if (z_attr(240, ATTR_INVISIBLE)) return shortcut ? art_z1_living_shortcut : art_living;
    if (z_attr(240, ATTR_OPEN)) return shortcut ? art_z1_hatch_open_shortcut : art_z1_hatch_open;
    return shortcut ? art_z1_living_moved_shortcut : art_living_moved;
}

static const Color *zork1_dam_scene(unsigned room) {
    bool low = z_global(14) != 0;
    bool sluices = z_global(56) != 0;
    if (room == 178) {
        if (low) return sluices ? art_z1_dam_low : art_z1_dam_refill;
        return sluices ? art_z1_dam_open : art_dam;
    }
    if (room == 123) return low ? art_z1_reservoir_south_dry : art_z1_reservoir_south;
    return low ? art_z1_reservoir_drained : art_z1_reservoir_north;
}

static const Color *zork1_location_art(void) {
    unsigned room = z_global(0);
    switch (room) {
        case 3: case 90: case 112: case 173: return art_z1_coal_mine;
        case 21: return art_z1_rocky_ledge;
        case 25: return art_z1_damp_cave;
        case 19: case 34: case 42: case 50: case 51: case 54: case 81: case 104: case 134:
        case 152: case 158: case 161: case 170: case 193: case 218: return art_z1_maze;
        case 141: case 143: case 147: case 153: return art_z1_maze_dead_end;
        case 62: case 176: return art_z1_tiny_cave;
        case 69: return art_z1_engraving_cave;
        case 74: return art_z1_canyon_bottom;
        case 106: return art_z1_chasm_south;
        case 130: return art_z1_east_west_passage;
        case 139: return art_z1_cold_passage;
        case 155: return art_z1_sandy_cave;
        case 175: return art_z1_north_south_passage;
        case 188: return art_z1_mine_entrance;
        case 228: return art_z1_narrow_passage;
        case 235: return art_z1_bat_room;
        case 247: return art_z1_chasm_east;
        case 2: return art_z1_temple;
        case 8: return art_z3_timber_room;
        case 5: return art_z1_tree_canopy;
        case 11: return art_z1_dome;
        case ZORK1_LADDER_TOP: return art_z1_ladder_top;
        case 18: return art_z1_torch_room;
        case 15: return art_z1_round_room;
        case 31: return art_z1_slide_room;
        case 27: return z_attr(243, ATTR_OPEN) ? art_kitchen : art_z1_kitchen_shut;
        case 30: return art_z1_sandy_beach;
        case 33: return z_attr(240, ATTR_OPEN) ? art_z1_cellar_hatch_open : art_z1_cellar_hatch;
        case 37:
        case 111: return art_z1_dam_base;
        case 40: return art_z1_shore;
        case 45: return art_z1_river_valley;
        case 67: return art_z1_stream_view;
        case 86: return art_z1_stream;
        case ZORK1_GAS_ROOM: return art_z1_gas_room;
        case 52: return art_z1_mirror_north;
        case 53: return art_z1_river_rapids;
        case ZORK1_LADDER_BOTTOM: return art_z1_ladder_bottom;
        case 75: return zork1_living_room();
        case 85: return z_attr(243, ATTR_OPEN) ? art_z1_rear_open : art_rear;
        case 89: return art_z1_machine_room;
        case 93: return art_z1_smelly_room;
        case 95:
        case 123:
        case 178:
        case 191: return zork1_dam_scene(room);
        // Both Mirror Room descriptions are identical and place the same
        // enormous mirror on the south wall. Reuse one construction so the
        // duplicated route rooms do not change stone/palette between visits.
        case 110: return art_z1_mirror_north;
        case 117: return art_z1_strange_passage;
        case 120: return art_z1_dam_lobby;
        case 122: return art_z1_gallery;
        case 162: return art_z1_deep_canyon;
        case 167: return art_z1_forest167;
        case 220: return art_z1_studio;
        case 129: return art_z1_altar;
        case 156: return art_z1_mine_dead_end;
        case 160:
            if (z_attr(29, ATTR_OPEN)) return art_z1_grate_clearing_open;
            if (z_attr(29, ATTR_INVISIBLE) && z_parent(157) == 160)
                return art_z1_grate_clearing_leaves;
            return art_z1_grate_clearing;
        case 171: return art_z1_squeaky_room;
        case 166:
        case 184: return art_z1_white_cliffs;
        case 174: return art_z1_river_bend;
        case 182: return art_z1_river_beach;
        case 238: return art_z1_forest_path;
        case 187: return art_z1_attic;
        case 202: return art_z1_rainbow_top;
        case 205: return art_z1_loud_room;
        case 216: return z_attr(29, ATTR_OPEN) ? art_z1_grate_cellar_open : art_z1_grate_cellar;
        case 224: return art_z1_maintenance;
        case 244: return zork1_barrow_finished() ? art_z1_barrow_inside : art_z1_barrow_outside;
        case 245: return art_z1_shaft;
        case 249: return art_z1_shaft_bottom;
        default: return NULL;
    }
}

bool zork2_fuse_burning(void) {
    // Release 63: C-TABLE is global27, forty 6-byte clock records.
    // I-FUSE is packed routine21855. Countdown zero means already fired.
    const uint8_t *data = z_dynamic();
    unsigned base = z_global(27), size = z_dynamic_size();
    if (base + 240 > size) return false;
    for (unsigned offset = 0; offset < 240; offset += 6) {
        const uint8_t *p = data + base + offset;
        unsigned enabled = p[0] * 256u + p[1], ticks = p[2] * 256u + p[3];
        if (enabled == 1 && ticks && ticks < 0x8000 && p[4] * 256u + p[5] == 21855) return true;
    }
    return false;
}

static bool zork2_crypt_has_light(void) {
    // The Crypt keeps the story's LIT global set to expose its faint secret-door cue.
    // Actual carried/local light sources still determine which artwork is appropriate.
    static const uint8_t sources[] = {23, 149, 181};
    for (unsigned i = 0; i < sizeof(sources) / sizeof(*sources); i++) {
        if (!z_attr(sources[i], ATTR_LIT)) continue;
        unsigned parent = z_parent(sources[i]);
        for (unsigned depth = 0; parent && depth < 16; depth++) {
            if (parent == 125 || parent == 40) return true;
            if (z_attr(parent, ATTR_CONTAINER) && !z_attr(parent, ATTR_OPEN) &&
                !z_attr(parent, ATTR_TRANSPARENT)) break;
            parent = z_parent(parent);
        }
    }
    return false;
}

static const Color *zork2_location_art(void) {
    unsigned room = z_global(0);
    switch (room) {
        case 224:
            // The Red Mist has a separate illustration for the story's
            // "You see only darkness" state; the normal view shows the room.
            return visual_state_mist_view_dark() ? art_z2_mist_red_dark : art_z2_mist_red;
        case 238: return z_attr(7, ATTR_OPEN) ? art_z2_riddle_room_238_open : art_z2_riddle_room_238;
        case 8: return art_z2_well_top;
        case 13: return z_global(72) ? art_z2_narrow_ledge_escape : art_z2_narrow_ledge;
        case 19: return z_global(13) ? art_z2_angled_entry_open : art_z2_angled_entry;
        // Same glassy room as 242; the floor window is drawn by room_visuals.c.
        case 95: case 142: case 185: case 226: return art_z2_angled_northeast;
        case 28: return z_attr(43, ATTR_OPEN) ? art_z2_guarded_open : art_z2_guarded_room;
        case 30: return z_global(163) ? art_z2_marble_open : art_z2_marble_hall;
        case 45: return z_attr(43, ATTR_OPEN) ? art_z2_workshop_open : art_z2_workshop;
        case 46: return z_attr(137, ATTR_OPEN) ? art_z2_tiny_open : art_z2_tiny_room;
        case 59: return z_attr(137, ATTR_OPEN) ? art_z2_dreary_open : art_z2_dreary_room;
        case 69: {
            static const Color *const looks[] = {art_z2_quarters_cave, art_z2_quarters_cave,
                art_z2_quarters_louis, art_z2_quarters_bunks, art_z2_quarters_hammock,
                art_z2_quarters_austere, art_z2_quarters_arabian};
            return looks[visual_state_quarters()];
        }
        case 82: return z_parent(39) == 82 ? art_z2_pool : art_z2_pool_dry;
        case 106: return art_z2_closet;
        case 125:
            if (zork2_crypt_has_light())
                return z_attr(151, ATTR_OPEN) ? art_z2_crypt_open : art_z2_crypt;
            return z_attr(151, ATTR_OPEN) ? art_z2_crypt_secret_open : art_z2_crypt_dark;
        case 161: return z_parent(111) == 161 ? art_z2_ice_room : art_z2_ice_melted;
        case 182: return z_parent(101) == 182 ? art_z2_safe_blast : art_z2_dusty_room;
        default: break;
    }
    static const struct { unsigned room; const Color *art; } scenes[] = {
#define ZORK2_SCENE(name, room, lx, ly, rx, ry) {room, art_z2_##name},
#include "zork2_scenes.def"
#undef ZORK2_SCENE
    };
    for (unsigned i = 0; i < sizeof(scenes) / sizeof(*scenes); i++)
        if (scenes[i].room && scenes[i].room == room) return scenes[i].art;
    return NULL;
}

static bool is_letter(char ch) { return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'); }

// True when `word` appears in `text` as a whole word (case-insensitive), optionally plural.
static bool has_word(const char *text, const char *word) {
    for (const char *p = text; *p; p++) {
        if (p != text && is_letter(p[-1])) continue;
        unsigned i = 0;
        while (word[i] && p[i] && lower(word[i]) == lower(p[i])) i++;
        if (word[i]) continue;
        if (lower(p[i]) == 's') i++;
        if (!is_letter(p[i])) return true;
    }
    return false;
}

static bool name_has_any(const char *text, const char *const *words, unsigned count) {
    for (unsigned i = 0; i < count; i++)
        if (has_word(text, words[i])) return true;
    return false;
}

const Color *story_scene(unsigned chapter, const char *room) {
    if (chapter == 0) {
        const Color *art = zork1_location_art();
        if (art) return art;
        static const struct { const char *name; const Color *image; } illustrated[] = {
            {"West of House", art_west}, {"North of House", art_north}, {"South of House", art_south},
            {"The Troll Room", art_troll_room},
            {"Canyon View", art_canyon}, {"Egyptian Room", art_egyptian},
            {"Treasure Room", art_treasure_room}, {"Atlantis Room", art_atlantis}, {"Bat Room", art_z1_bat_room},
            {"Cyclops Room", art_cyclops_room}, {"Aragain Falls", art_aragain_falls},
            {"End of Rainbow", art_end_of_rainbow}, {"Entrance to Hades", art_hades},
            {"Land of the Dead", art_hades_inside},
        };
        for (unsigned i = 0; i < sizeof(illustrated) / sizeof(illustrated[0]); i++)
            if (text_equal(room, illustrated[i].name)) return illustrated[i].image;
    } else if (chapter == 1) {
        const Color *art = zork2_location_art();
        if (art) return art;
        if (z_global(0) == ZORK2_AQUARIUM_ROOM)
            return z_parent(ZORK2_SERPENT) == ZORK2_AQUARIUM ? art_aquarium : art_aquarium_broken;
        if (text_equal(room, "Inside the Barrow")) return art_barrow;
        if (text_equal(room, "Riddle Room")) return z_attr(7, ATTR_OPEN) ? art_riddle_room_open : art_riddle_room;
        if (text_equal(room, "Carousel Room")) return art_carousel;
        if (text_equal(room, "Tea Room")) return art_tea;
        if (text_equal(room, "Great Cavern")) return art_great_cavern;
        if (text_equal(room, "Wizard's Workroom")) return art_workroom;
        if (text_equal(room, "Pentagram Room")) return art_pentagram_room;
        if (text_equal(room, "Vault")) return art_bank_vault;
        if (z_global(0) == ZORK2_BANK_SMALL_ROOM) return art_bank_small_room;
        if (text_equal(room, "Safety Depository")) return art_bank_depository;
        if (text_equal(room, "Cerberus Room")) return art_cerberus_room;
        if (text_equal(room, "Crypt Anteroom"))
            return z_attr(ZORK2_CRYPT_DOOR, ATTR_OPEN) ? art_crypt_anteroom_open : art_crypt_anteroom;
        if (text_equal(room, "Menhir Room")) return art_menhir_room;
        if (text_equal(room, "Dragon Room")) return art_dragon_room;
        if (text_equal(room, "Machine Room")) return art_robot_machine;
    } else if (chapter == 2) {
        unsigned here = z_global(0);
        if (here == 55) return art_z3_creepy_crawl;
        if (here == ZORK3_LAND_OF_SHADOW_69) return art_z3_land_of_shadow_69;
        if (here == 170) return art_z3_shadow_cliff;   // clifftop over the ocean, ruined wall to the north
        if (here == 39) return art_z3_key_room;
        if (here == 40) return art_z3_tight_squeeze;
        if (here == 43) return art_z3_east_corridor;
        if (here == 78) return art_z3_hairpin_loop;
        if (here == 86) return art_z3_dead_end;
        if (here == 112) return art_z3_barren_area;
        if (here == 113 || here == ZORK3_GUARDIANS_HALL || here == ZORK3_GUARDIANS_NORTH)
            return art_z3_guardian_hallway_v3;
        if (here == ZORK3_GUARDIANS_SOUTH) return art_z3_mirror_hall_closed;
        if (here == 159) return art_z3_narrow_corridor;
        if (here == 162) return art_z3_west_corridor;
        if (here == 190) return art_z3_south_corridor;
        if (here == 194) return art_z3_puzzle_room;
        if (here == ZORK3_LADDER_TOP) return art_z3_ladder_top;
        if (here == ZORK3_LADDER_BOTTOM) return art_z3_ladder_bottom;
        if (here == ZORK3_MINE_DEAD_END) return art_z3_mine_dead_end;
        if (here == ZORK3_SIDE_ROOM || here == ZORK3_SIDE_ROOM_ALT) return art_z3_side_room;
        if (here == ZORK3_UNDERWATER) return art_z3_underwater;
        if (here == ZORK3_LAKE) return art_z3_lake_surface;
        if (here == ZORK3_WEST_SHORE) return art_z3_west_shore;
        if (here == ZORK3_SOUTH_SHORE) return art_z3_south_shore;
        if (here == ZORK3_AQUEDUCT) return art_z3_aqueduct;
        if (here == ZORK3_HIGH_ARCH) return art_z3_high_arch;
        if (here == ZORK3_WATER_SLIDE) return art_z3_water_slide;
        if (here == ZORK3_DAMP_PASSAGE) return art_z3_damp_passage;
        if (here == ZORK3_AQUEDUCT_VIEW) return art_z3_aqueduct_view;
        if (here == ZORK3_SCENIC_VISTA) return art_z3_scenic_vista;
        if (here == ZORK3_ROOM_8) return art_z2_room8;
        if (here == ZORK3_TECHNOLOGY_PAST) return art_z3_technology_unfinished;
        if (here == ZORK3_JEWEL_PAST) return art_z3_jewel_unfinished;
        if (here == ZORK3_TREASURY) return art_z3_treasury;
        if (here == ZORK3_CRYSTAL_GROTTO) return art_z3_crystal_grotto;
        if (here == ZORK3_FOGGY_ROOM) return art_z3_foggy_room;
        if (here == ZORK3_GREAT_DOOR) return art_z3_great_door;
        if (here == ZORK3_ENDLESS_STAIR) return art_z3_endless_stair;
        if (here == ZORK3_NORTH_CORRIDOR)
            return z_attr(183, ATTR_OPEN) ? art_z3_marble_hall_prison_open : art_z3_marble_hall;
        if (here == ZORK3_BEAM_ROOM) return art_z3_beam_room;
        if (here == ZORK3_BUTTON_ROOM) return art_z3_button_room;
        if (here == ZORK3_TIMBER_ROOM) return art_z3_timber_room;
        if (here == ZORK3_PARAPET) return art_z3_parapet;
        if (here == 169)
            return zork3_mirror_panel_open() ? art_z3_mirror_hall_open : art_z3_mirror_hall_closed;
        if (here == 45) return z_attr(183, ATTR_OPEN) ? art_z3_prison_cell_open : art_z3_prison_cell_closed;
        if (here == 204) return art_z3_prison_cell_bronze;   // "On the south wall is a bronze door"
        if (here == 225) return art_z3_prison_cell_closed;
        if (here == 84 || here == 143) return art_z3_royal_puzzle_entrance;
        if (here == ZORK3_ROYAL_HALL) return art_z3_royal_hall;
        if (text_equal(room, "Royal Hall")) return art_z3_royal_hall;
        if (text_equal(room, "Prison Cell")) return art_z3_prison_cell_closed;
        if (text_equal(room, "Narrow Room")) {
            const Color *narrow = art_z3_narrow_mirror;
            if (here == ZORK3_NARROW_GUARDIANS_EAST || here == ZORK3_NARROW_GUARDIANS_WEST)
                narrow = art_z3_narrow_mirror_guardians;
            else if (here == ZORK3_NARROW_STATUES_EAST || here == ZORK3_NARROW_STATUES_WEST)
                narrow = art_z3_narrow_mirror_guardians_north;
            if (here == 51 || here == 151 || here == 161 || here == 221) {
                if (narrow == art_z3_narrow_mirror_guardians)
                    return art_z3_narrow_mirror_guardians_west;
                if (narrow == art_z3_narrow_mirror_guardians_north)
                    return art_z3_narrow_mirror_guardians_north_west;
                return art_z3_narrow_mirror_west;
            }
            return narrow;
        }
        if (text_equal(room, "Endless Stair")) return art_stairway;
        if (text_equal(room, "Junction")) return art_junction;
        if (text_equal(room, "Technology Museum")) return art_technology_museum;
        if (text_equal(room, "Museum Entrance"))
            return z_attr(205, ATTR_OPEN) ? art_museum_entrance_open : art_museum_entrance;
        if (text_equal(room, "Inside Mirror")) return art_mirror_room_v2;
        if (text_equal(room, "Parapet")) return art_z3_parapet;
        if (text_equal(room, "Lake Shore")) return art_lake_shore;
        if (text_equal(room, "Jewel Room")) return art_jewel_room;
        if (text_equal(room, "Cliff Ledge")) return art_cliff_ledge_upper_shelf;
        if (text_equal(room, "Cliff")) return art_cliff_top;
        if (text_equal(room, "Cliff Base")) return art_cliff_base;
        if (text_equal(room, "Engravings Room")) return art_engravings;
        if (text_equal(room, "Land of Shadow")) return art_land_of_shadow;
        if (text_equal(room, "Dungeon Entrance")) return art_dungeon_entrance;
        if (text_equal(room, "Flathead Ocean"))
            return z_attr(ZORK3_VIKING_SHIP, ZORK3_SHIP_HIDDEN) ? art_flathead_ocean : art_flathead_ship;
    }
    // Generic art for rooms without dedicated scenes, chosen by whole words in the room name
    // (an optional plural "s" allowed), so "Damp Cave" is a cave rather than a dam.
    static const char *const forest[] = {"forest", "clearing", "tree"};
    static const char *const water[] = {"river", "water", "lake", "shore", "ford", "reservoir", "bridge", "stream"};
    static const char *const cave[] = {"cave", "crawl", "tunnel", "maze", "mine", "cavern", "chasm", "ledge",
                                       "passage", "ravine", "canyon"};
    if (name_has_any(room, forest, sizeof(forest) / sizeof(*forest))) return art_forest;
    if (name_has_any(room, water, sizeof(water) / sizeof(*water))) return art_water;
    if (name_has_any(room, cave, sizeof(cave) / sizeof(*cave))) return art_cavern;
    return art_hall;
}

static void raw_noun(unsigned id, char buffer[Z_NAME_MAX]) {
    static const char *const extra[] = {"me", "north wall", "east wall", "south wall", "west wall"};
    if (id >= TARGET_SELF && id <= TARGET_WEST_WALL) {
        buffer[0] = 0;
        append_text(buffer, Z_NAME_MAX, extra[id - TARGET_SELF]);
    } else z_name(id, buffer, Z_NAME_MAX);
}

void story_noun(unsigned id, char buffer[Z_NAME_MAX]) {
    static const struct { const char *name, *noun; } aliases[] = {
        {"matchbook", "match"}, {"pair of candles", "candles"},
        {"quantity of water", "water"}, {"quantity of salty water", "salty water"},
        {"china teapot", "teapot"}, {"wooden bucket", "bucket"}, {"pool of tears", "pool"},
        {"shiny object", "object"}, {"pile of plastic", "plastic"}, {"hand-held air pump", "pump"},
        {"Wizard's magic wand", "wand"}, {"Wizard of Frobozz", "wizard"}, {"enormous menhir", "menhir"},
        {"Frobozz Magic Grue Repellent", "repellent"}, {"red hot brass bell", "bell"},
        {"jewel-encrusted egg", "egg"}, {"very ancient book", "book"},
        {"cake frosted with green letters", "green cake"}, {"cake frosted with blue letters", "blue cake"},
        {"cake frosted with orange letters", "orange cake"}, {"cake frosted with red letters", "red cake"},
        {"red crystal sphere", "red sphere"}, {"blue crystal sphere", "blue sphere"},
        {"clear crystal sphere", "clear sphere"}, {"black crystal sphere", "black sphere"},
        {"door made of oak", "door"}, {"door partly covered in cobwebs", "door"},
        {"golden dragon statuette", "statuette"}, {"portrait of J. Pierpont Flathead", "portrait"},
        {"gigantic dog collar", "collar"}, {"three-headed dog", "cerberus"},
        {"priceless zorkmid", "coin"}, {"heavy but invisible liquid", "potion"},
        {"huge red dragon", "dragon"},
        {"number of ghosts", "ghosts"}, {"Gnome of Zurich", "gnome"}
    };
    raw_noun(id, buffer);
    for (unsigned i = 0; i < sizeof(aliases) / sizeof(aliases[0]); i++) {
        if (!text_equal(buffer, aliases[i].name)) continue;
        buffer[0] = 0;
        append_text(buffer, Z_NAME_MAX, aliases[i].noun);
        return;
    }
}

// A token the dictionary can't match makes the story print "I don't know the
// word ...". Ask the VM's tokenizer directly so its six-Z-character limit and
// punctuation separators stay in sync with the parser.
typedef struct { char token[Z_NAME_MAX]; bool known; } DictionaryToken;
static DictionaryToken dictionary_tokens[16];
static unsigned dictionary_token_generation;

static bool dictionary_has(const char *token) {
    unsigned generation = z_story_generation();
    if (generation != dictionary_token_generation) {
        dictionary_token_generation = generation;
        for (unsigned i = 0; i < sizeof(dictionary_tokens) / sizeof(*dictionary_tokens); i++) {
            dictionary_tokens[i].token[0] = 0;
            dictionary_tokens[i].known = false;
        }
    }
    unsigned slot = 0;
    for (const char *p = token; *p; p++) slot = (slot * 31u + (uint8_t)lower(*p)) & 15u;
    DictionaryToken *entry = &dictionary_tokens[slot];
    if (entry->token[0] && text_equal(entry->token, token)) return entry->known;
    bool known = z_dictionary_phrase_known(token);
    entry->token[0] = 0;
    append_text(entry->token, sizeof(entry->token), token);
    entry->known = known;
    return known;
}

void story_parser_noun(unsigned id, char buffer[Z_NAME_MAX]) {
    buffer[0] = 0;
    char source[Z_NAME_MAX];
    story_noun(id, source);
    char kept[Z_NAME_MAX] = {0};
    unsigned used = 0;
    bool first = true;
    const char *cursor = source;
    while (*cursor) {
        while (*cursor == ' ') cursor++;
        if (!*cursor) break;
        const char *start = cursor;
        while (*cursor && *cursor != ' ') cursor++;
        char token[Z_NAME_MAX] = {0};
        unsigned len = (unsigned)(cursor - start);
        if (len >= sizeof(token)) len = sizeof(token) - 1;
        for (unsigned i = 0; i < len; i++) token[i] = start[i];
        if (dictionary_has(token)) {
            if (!first && used + 1 < sizeof(kept)) kept[used++] = ' ';
            for (unsigned i = 0; i < len && used + 1 < sizeof(kept); i++) kept[used++] = token[i];
            kept[used] = 0;
            first = false;
        }
    }
    // An object with no known noun cannot be addressed by the story parser.
    // Leave it out of focus rather than offering an action that will fail at
    // tokenization.
    if (used) append_text(buffer, Z_NAME_MAX, kept);
}

void story_name(unsigned chapter, unsigned id, char buffer[Z_NAME_MAX]) {
    raw_noun(id, buffer);
    if (id == TARGET_SELF) {
        buffer[0] = 0;
        append_text(buffer, Z_NAME_MAX, "Yourself");
    } else if (chapter && text_equal(buffer, "lamp")) {
        buffer[0] = 0;
        append_text(buffer, Z_NAME_MAX, "Brass lantern");
    } else if (buffer[0] >= 'a' && buffer[0] <= 'z') {
        buffer[0] -= 'a' - 'A';
    }
}

static bool add_object(Targets *list, unsigned id, unsigned chapter) {
    unsigned player = chapters[chapter].player;
    if (list->count == TARGET_LIMIT || !id || id > z_object_count() || id == player) return false;
    if (chapter == 0 && id == ZORK1_TROLL && visual_state_troll_disappeared()) return false;
    // Zork II uses different INVISIBLE and TRANSBIT attributes.
    if (z_attr(id, chapter == 1 ? 9 : ATTR_INVISIBLE)) return false;
    if (chapter == 2 && id == ZORK3_VIKING_SHIP && z_attr(id, ZORK3_SHIP_HIDDEN)) return false;
    if (chapter == 2 && id == ZORK3_CLIFF_MAN && !story_cliff_man_visible()) return false;
    if (chapter == 2 && id == 11 && z_global(0) == ZORK3_DUNGEON_ENTRANCE) return false;
    char name[Z_NAME_MAX];
    z_name(id, name, sizeof(name));
    if (!*name) return false;
    for (unsigned i = 0; i < list->count; i++) {
        if (list->id[i] == id) return false;
        // Zork III has both an actor and a room-local proxy for the Dungeon Master.
        if (chapter == 2 && ((id == 1 && list->id[i] == 11) || (id == 11 && list->id[i] == 1))) return false;
    }
    char noun[Z_NAME_MAX];
    story_parser_noun(id, noun);
    if (!*noun) return true; // Don't focus it, but still allow visible contents through it.
    list->id[list->count++] = id;
    return true;
}

static void add_contents(Targets *list, unsigned parent, unsigned chapter, unsigned depth) {
    unsigned vessel = z_parent(chapters[chapter].player);
    unsigned transparent = chapter == 1 ? 7 : ATTR_TRANSPARENT;
    unsigned item = z_child(parent);
    for (unsigned i = 0; item && i < z_object_count(); i++, item = z_sibling(item)) {
        bool visible = add_object(list, item, chapter);
        if (visible && depth &&
            (z_attr(item, ATTR_OPEN) || z_attr(item, transparent) || item == vessel))
            add_contents(list, item, chapter, depth - 1);
    }
}

// A room's local globals are scenery it mentions, not things that are always
// there: Zork I's forest lists the songbird you only hear and the house you can
// only see from afar, and Zork III's lake lists a fish that comes and goes.
// Ask the story itself, then rewind, so only scenery it will discuss is offered.
static bool story_rejects(unsigned id) {
    static const char *const refusals[] = {"You can't see any ", "You're not at ", "There is no "};
    char noun[Z_NAME_MAX], line[Z_NAME_MAX + 8] = "examine ", reply[32];
    story_parser_noun(id, noun);
    append_text(line, sizeof(line), noun);
    if (!z_probe(line, reply, sizeof(reply), 20000)) return false;
    const char *text = reply;
    while (*text == '\n' || *text == ' ' || *text == '>') text++;
    for (unsigned i = 0; i < sizeof(refusals) / sizeof(*refusals); i++) {
        const char *a = text, *b = refusals[i];
        while (*b && *a == *b) a++, b++;
        if (!*b) return true;
    }
    return false;
}

void story_targets(unsigned chapter, Targets *room, Targets *bag) {
    unsigned here = z_global(0), player = chapters[chapter].player;
    room->count = bag->count = 0;
    add_contents(bag, player, chapter, 4);
    if (story_dark(chapter)) {
        // The outline of the crypt door is visible by its glow.
        if (chapter == 1 && here == 125) add_object(room, 151, chapter);
        return;
    }
    add_contents(room, here, chapter, 4);
    const uint8_t *locals;
    unsigned n = z_property(here, chapters[chapter].local_prop, &locals);
    bool puzzle = chapter == 2 && here == ROYAL_PUZZLE_ROOM;
    for (unsigned i = 0; i < n; i++) {
        unsigned id = locals[i];
        if (puzzle && (id == 42 || id == 104 || id == 115 || id == 187)) continue;
        unsigned before = room->count;
        add_object(room, id, chapter);
        if (room->count != before && story_rejects(id)) room->count--;
    }
    if (puzzle) {
        for (unsigned id = TARGET_NORTH_WALL; id <= TARGET_WEST_WALL && room->count < TARGET_LIMIT; id++)
            room->id[room->count++] = id;
    }
}

unsigned story_focus_dropped_item(const Targets *previous_bag, unsigned room, const Targets *current_room) {
    for (unsigned i = 0; i < previous_bag->count; i++) {
        unsigned object = previous_bag->id[i];
        if (z_parent(object) != room) continue;
        for (unsigned j = 0; j < current_room->count; j++)
            if (current_room->id[j] == object) return j;
    }
    return current_room->count;
}
