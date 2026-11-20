#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "interaction.h"
#include "puzzle_map.h"
#include "room_map.h"
#include "save.h"
#include "transcript_codec.h"
#include "visual_state.h"
#include "zork1_scenes.h"

extern const Color art_living_moved[];

static uint8_t story[96 * 1024], sram[SRAM_SIZE], visited[VISITED_BYTES];
static unsigned chapter;
static Trail trail;
static PuzzleMap saved_map;
static char captured_story_text[8192];
static unsigned captured_story_size, replay_plain_bytes, replay_encoded_bytes;
static bool capture_overflow;
static uint8_t replay_corpus[32768];
static unsigned replay_corpus_size;
static bool replay_corpus_overflow;

static void ignore_text(const char *s, size_t size) {
    visual_state_observe(chapter, z_global(0), s, size);
    if (size > sizeof(captured_story_text) - captured_story_size) capture_overflow = true;
    else {
        memcpy(captured_story_text + captured_story_size, s, size);
        captured_story_size += (unsigned)size;
    }
}
static void fatal(const char *reason, unsigned pc, unsigned opcode) {
    fprintf(stderr, "%s (PC %x, opcode %x)\n", reason, pc, opcode);
    abort();
}
static uint32_t entropy(void) { return 7; }
static bool save_vm(void) { return save_write(sram, chapter, visited, &trail, &saved_map); }
static bool restore_vm(void) { return save_read(sram, chapter, visited, &trail, &saved_map); }
static const ZPlatform io = {ignore_text, fatal, entropy, save_vm, restore_vm};

static void start(unsigned game) {
    char path[32];
    snprintf(path, sizeof(path), "stories/zork%u.z3", game + 1);
    FILE *file = fopen(path, "rb");
    assert(file);
    size_t length = fread(story, 1, sizeof(story), file);
    assert(!ferror(file));
    fclose(file);
    chapter = game;
    trail.count = 0;
    saved_map.valid = false;
    memset(visited, 0, sizeof(visited));
    visual_state_reset();
    z_start(story, length, &io);
    assert(z_run(1000000) == Z_PROMPT);
}

static void command(const char *line) {
    captured_story_size = 0;
    capture_overflow = false;
    char history_line[COMMAND_SIZE + 4];
    unsigned history_length = 0;
    history_line[history_length++] = '>';
    history_line[history_length++] = ' ';
    while (line[history_length - 2] && history_length < sizeof(history_line) - 1) {
        history_line[history_length] = line[history_length - 2];
        history_length++;
    }
    history_line[history_length++] = '\n';
    unsigned encoded_command_size = transcript_codec_encoded_size(history_line, history_length);
    assert(z_submit(line));
    assert(z_run(1000000) == Z_PROMPT);
    while (captured_story_size &&
           (captured_story_text[captured_story_size - 1] == '\n' ||
            captured_story_text[captured_story_size - 1] == ' ' ||
            captured_story_text[captured_story_size - 1] == '>')) captured_story_size--;
    if (!capture_overflow) {
        replay_plain_bytes += history_length + captured_story_size;
        replay_encoded_bytes += encoded_command_size +
            transcript_codec_encoded_size(captured_story_text, captured_story_size);
        unsigned record_size = history_length + captured_story_size + 1;
        if (record_size <= sizeof(replay_corpus) - replay_corpus_size) {
            memcpy(replay_corpus + replay_corpus_size, history_line, history_length);
            replay_corpus_size += history_length;
            memcpy(replay_corpus + replay_corpus_size, captured_story_text, captured_story_size);
            replay_corpus_size += captured_story_size;
            replay_corpus[replay_corpus_size++] = 0;
        } else replay_corpus_overflow = true;
    }
}

static void dump_replay_corpus(void) {
    const char *path = getenv("ZORK_TRANSCRIPT_DUMP");
    if (!path || !*path) return;
    assert(!replay_corpus_overflow);
    FILE *file = fopen(path, "wb");
    assert(file);
    assert(fwrite(replay_corpus, 1, replay_corpus_size, file) == replay_corpus_size);
    assert(!fclose(file));
}

static void check_dictionary(const char *const expected[], unsigned count) {
    unsigned found = 0, size = z_dictionary_size();
    char word[Z_WORD_MAX];
    assert(size > 500);
    for (unsigned i = 0; i < size; i++) {
        z_dictionary_word(i, word);
        for (unsigned j = 0; j < count; j++) {
            if (!strcmp(word, expected[j])) found |= 1u << j;
        }
    }
    z_dictionary_word(size, word);
    assert(!*word && found == (1u << count) - 1);
}

static bool has_target(const Targets *targets, unsigned id) {
    for (unsigned i = 0; i < targets->count; i++) if (targets->id[i] == id) return true;
    return false;
}

static void check_transcript_codec(void) {
    const char sample[] = "> look\nYou are in a small room. The door is open.\n"
                          "There is nothing behind the door.\nYou can't see the floor.\n"
                          "That would be quite odd.\n be \n";
    uint8_t encoded[512];
    char expanded[512];
    unsigned encoded_size = 0, expanded_size = 0;
    unsigned sample_size = strlen(sample);
    assert(transcript_codec_encoded_size(sample, sample_size) < sample_size);
    assert(transcript_codec_encode(sample, sample_size, encoded, sizeof(encoded), &encoded_size));
    assert(transcript_codec_expand(encoded, encoded_size, expanded, sizeof(expanded), &expanded_size));
    assert(expanded_size == sample_size && !memcmp(sample, expanded, sample_size) && !expanded[expanded_size]);
    unsigned ignored_size = 0;
    assert(!transcript_codec_encode(sample, sample_size, encoded, encoded_size - 1, &ignored_size));
    printf("Transcript sample: %u -> %u bytes (%u%% saved).\n", sample_size, encoded_size,
           (sample_size - encoded_size) * 100 / sample_size);

    const char unusual[] = {(char)0x1b, (char)0x80, (char)0xff, 'x'};
    assert(transcript_codec_encode(unusual, sizeof(unusual), encoded, sizeof(encoded), &encoded_size));
    assert(transcript_codec_expand(encoded, encoded_size, expanded, sizeof(expanded), &expanded_size));
    assert(expanded_size == sizeof(unusual) && !memcmp(unusual, expanded, sizeof(unusual)));

    const uint8_t truncated_escape[] = {0x1b};
    assert(!transcript_codec_expand(truncated_escape, sizeof(truncated_escape), expanded,
                                    sizeof(expanded), &expanded_size));
}

static void check_transcript_save(void) {
    const char *const entries[] = {
        "> look\nYou are in a small room. The door is open.\n",
        "> wait\nThere is nothing behind the door.\n"
    };
    uint8_t packed[256], restored[256];
    char expanded[256], expanded_tail[8192];
    unsigned packed_size = 0;
    for (unsigned i = 0; i < sizeof(entries) / sizeof(entries[0]); i++) {
        unsigned written = 0, length = strlen(entries[i]);
        assert(transcript_codec_encode(entries[i], length, packed + packed_size,
                                       sizeof(packed) - packed_size - 1, &written));
        packed_size += written;
        packed[packed_size++] = 0;
    }

    Trail expected_trail = {0}, restored_trail = {0};
    trail_record(&expected_trail, 64, 1, 77);
    trail_record(&expected_trail, 77, 2, 64);
    PuzzleMap map = {0}, restored_map = {0};
    assert(save_write_with_transcript(sram, chapter, visited, &expected_trail, &map,
                                      packed, packed_size, false));
    unsigned restored_size = 0;
    bool trimmed = true;
    assert(save_read_with_transcript(sram, chapter, visited, &restored_trail, &restored_map,
                                    restored, sizeof(restored), &restored_size, &trimmed));
    assert(restored_size == packed_size && !memcmp(packed, restored, packed_size) && !trimmed);
    assert(restored_trail.count == expected_trail.count &&
           trail_destination(&restored_trail, 64, 1) == 77 &&
           trail_destination(&restored_trail, 77, 2) == 64);

    unsigned pos = 0;
    for (unsigned i = 0; i < sizeof(entries) / sizeof(entries[0]); i++) {
        unsigned length = 0, expanded_size = 0;
        while (pos + length < restored_size && restored[pos + length]) length++;
        assert(transcript_codec_expand(restored + pos, length, expanded, sizeof(expanded), &expanded_size));
        assert(expanded_size == strlen(entries[i]) && !memcmp(expanded, entries[i], expanded_size));
        pos += length + 1;
    }
    assert(pos == restored_size);

    // A single oversized newest entry should be stored from a safe encoded boundary.
    char long_entry[7000];
    unsigned long_prefix = sizeof(long_entry) - sizeof("LAST MARKER.");
    for (unsigned i = 0; i < long_prefix; i++) {
        if (i % 97 == 0) long_entry[i] = 0x1b;
        else if (i % 97 == 1) long_entry[i] = (char)0xfe;
        else long_entry[i] = "QWERTYUIOPASDFGHJKLZXCVBNM"[i % 26];
    }
    strcpy(long_entry + long_prefix, "LAST MARKER.");
    unsigned long_length = sizeof(long_entry) - 1;
    uint8_t oversized[8192], oversized_restored[8192];
    unsigned oversized_size = 0;
    assert(transcript_codec_encode(long_entry, long_length, oversized, sizeof(oversized) - 1,
                                   &oversized_size));
    oversized[oversized_size++] = 0;
    assert(save_write_with_transcript(sram, chapter, visited, &expected_trail, &map,
                                      oversized, oversized_size, false));
    restored_size = 0;
    trimmed = false;
    assert(save_read_with_transcript(sram, chapter, visited, &restored_trail, &restored_map,
                                    oversized_restored, sizeof(oversized_restored),
                                    &restored_size, &trimmed));
    assert(trimmed && restored_size > 0 && restored_size < oversized_size);
    assert(restored_trail.count == expected_trail.count &&
           trail_destination(&restored_trail, 64, 1) == 77 &&
           trail_destination(&restored_trail, 77, 2) == 64);
    unsigned tail_length = 0;
    while (tail_length < restored_size && oversized_restored[tail_length]) tail_length++;
    assert(tail_length + 1 == restored_size);
    unsigned expanded_tail_size = 0;
    assert(transcript_codec_expand(oversized_restored, tail_length, expanded_tail, sizeof(expanded_tail),
                                  &expanded_tail_size));
    assert(expanded_tail_size >= sizeof("LAST MARKER.") - 1 &&
           !strcmp(expanded_tail + expanded_tail_size - (sizeof("LAST MARKER.") - 1), "LAST MARKER."));
}

static void check_action(unsigned object, ActionGroup group, const char *name, const char *item, const char *expected) {
    Action actions[MAX_ACTIONS];
    unsigned count = story_actions(chapter, object, false, group, actions);
    unsigned row = 0;
    while (row < count && strcmp(actions[row].name, name)) row++;
    assert(row < count);
    char line[COMMAND_SIZE];
    assert(action_command(actions[row], object, item, line));
    assert(!strcmp(line, expected));
}

// Every object's action list must fit in MAX_ACTIONS without story_actions' cap kicking in.
static void check_action_capacity(unsigned story) {
    static const ActionGroup groups[] = {PRIMARY, INSPECT, HANDLE, USE, COMBINE, TOOLS, MOVE, ORDERS};
    unsigned peak = 0;
    for (unsigned id = 1; id <= z_object_count(); id++) {
        for (unsigned carried = 0; carried < 2; carried++) {
            for (unsigned g = 0; g < sizeof(groups) / sizeof(groups[0]); g++) {
                Action actions[MAX_ACTIONS];
                unsigned count = story_actions(story, id, carried, groups[g], actions);
                assert(count <= MAX_ACTIONS);
                if (groups[g] == PRIMARY) {
                    assert(count < MAX_ACTIONS);  // Headroom: the conditional cap was never reached.
                    assert(!strcmp(actions[count - 1].name, "More actions..."));
                    if (count > peak) peak = count;
                }
            }
        }
    }
    printf("Story %u primary action peak: %u of %u.\n", story + 1, peak, MAX_ACTIONS);
}

static void check_gazebo_targets(void) {
    const char *const route[] = {"turn on lamp", "s", "s", "s", "sw", "s", "se", "enter gazebo"};
    for (unsigned i = 0; i < sizeof(route) / sizeof(route[0]); i++) command(route[i]);
    assert(z_global(0) == 16);

    const unsigned teapot = 107;
    Targets room, bag;
    story_targets(1, &room, &bag);
    assert(has_target(&room, teapot));
    Action actions[MAX_ACTIONS];
    unsigned count = story_actions(1, teapot, false, PRIMARY, actions);
    unsigned take = 0;
    while (take < count && strcmp(actions[take].name, "Take")) take++;
    assert(take < count);
    char line[COMMAND_SIZE];
    assert(action_command(actions[take], teapot, NULL, line));
    command(line);
    story_targets(1, &room, &bag);
    assert(has_target(&bag, teapot) && !has_target(&room, teapot));
}

static void check_first_zork_dam(void) {
    const char *const approach[] = {"s", "e"};
    for (unsigned i = 0; i < sizeof(approach) / sizeof(*approach); i++) command(approach[i]);
    const Color *ajar = story_scene(0, "Behind House");
    command("open window");
    assert(story_scene(0, "Behind House") == art_z1_rear_open && ajar != art_z1_rear_open);
    const char *const house[] = {
        "enter window", "open sack", "take garlic", "w", "take lamp", "take sword", "turn on lamp", "move rug"
    };
    for (unsigned i = 0; i < sizeof(house) / sizeof(*house); i++) command(house[i]);
    assert(story_scene(0, "Living Room") == art_living_moved);
    command("open trap door");
    assert(story_scene(0, "Living Room") == art_z1_hatch_open);
    const char *const below[] = {
        "down", "n", "kill troll with sword", "kill troll with sword", "kill troll with sword",
        "kill troll with sword", "kill troll with sword", "e", "e", "n", "n", "ne", "e", "down"
    };
    for (unsigned i = 0; i < sizeof(below) / sizeof(*below); i++) command(below[i]);
    assert(z_global(0) == 111 && z_parent(219) == 111);
    command("swim");
    assert(z_global(0) == 111 && z_parent(219) == 111);
    command("look");
    const char *const mine[] = {
        "up", "north", "east", "take wrench", "press yellow button", "west", "south",
        "turn bolt with wrench", "west", "wait", "wait", "wait", "north", "north", "north",
        "up", "north", "north", "west", "north", "west", "north", "east", "north", "down"
    };
    for (unsigned i = 0; i < sizeof(mine) / sizeof(*mine); i++) command(mine[i]);
    assert(z_global(0) == ZORK1_GAS_ROOM && story_scene(0, "Gas Room") == art_z1_gas_room);
    check_action(ZORK1_STAIRS, PRIMARY, "Climb up", NULL, "climb up stairs");
    command("take sapphire-encrusted bracelet");
    assert(z_parent(ZORK1_BRACELET) == 44 && save_vm());
    command("up");
    command("down");
    assert(z_global(0) == ZORK1_GAS_ROOM && z_parent(ZORK1_BRACELET) == 44);
    Targets before_drop, carried_before_drop, after_drop, carried_after_drop;
    story_targets(0, &before_drop, &carried_before_drop);
    assert(has_target(&carried_before_drop, ZORK1_BRACELET));
    command("drop sapphire-encrusted bracelet");
    assert(z_parent(ZORK1_BRACELET) == ZORK1_GAS_ROOM);
    story_targets(0, &after_drop, &carried_after_drop);
    unsigned drop_focus = story_focus_dropped_item(&carried_before_drop, z_global(0), &after_drop);
    assert(drop_focus < after_drop.count && after_drop.id[drop_focus] == ZORK1_BRACELET);
    assert(restore_vm() && z_parent(ZORK1_BRACELET) == 44);
    const char *const ladder[] = {"east", "ne", "se", "sw", "down"};
    for (unsigned i = 0; i < sizeof(ladder) / sizeof(*ladder); i++) command(ladder[i]);
    assert(z_global(0) == ZORK1_LADDER_TOP && story_scene(0, "Ladder Top") == art_z1_ladder_top);
    check_action(ZORK1_MINE_LADDER, PRIMARY, "Climb down", NULL, "climb down wooden ladder");
    check_action(ZORK1_MINE_LADDER, MOVE, "Climb down", NULL, "climb down wooden ladder");
    command("climb down wooden ladder");
    assert(z_global(0) == ZORK1_LADDER_BOTTOM && story_scene(0, "Ladder Bottom") == art_z1_ladder_bottom);
    check_action(ZORK1_MINE_LADDER, PRIMARY, "Climb up", NULL, "climb up wooden ladder");
    check_action(ZORK1_MINE_LADDER, MOVE, "Climb up", NULL, "climb up wooden ladder");
    command("climb up wooden ladder");
    assert(z_global(0) == ZORK1_LADDER_TOP);
    check_action(ZORK1_STAIRS, PRIMARY, "Climb up", NULL, "climb up stairs");
    check_action(ZORK1_STAIRS, MOVE, "Climb up", NULL, "climb up stairs");
    command("climb up stairs");
    assert(z_global(0) == 3);
}

static void check_puzzle_map(void) {
    PuzzleMap map = {0};
    char response[] = "Room in a Puzzle\n"
                      "      +MM MM ??+\n"
                      "West  +   .. SS+  East\n"
                      "      +SS    MM+\n"
                      "There is a ladder on the east wall.\n";
    unsigned size = strlen(response);
    puzzle_map_update(&map, response, &size);
    assert(map.valid);
    assert(map.tiles[0] == PUZZLE_MARBLE && map.tiles[2] == PUZZLE_UNKNOWN);
    assert(map.tiles[4] == PUZZLE_PLAYER && map.tiles[5] == PUZZLE_SANDSTONE);
    assert(map.tiles[7] == PUZZLE_FLOOR);
    assert(!strcmp(response, "Room in a Puzzle\nThere is a ladder on the east wall.\n"));
    assert(size == strlen(response));

    char partial[] = "      +MM MM ??+\nWest  +   .. SS+";
    size = strlen(partial);
    puzzle_map_update(&map, partial, &size);
    assert(map.tiles[7] == PUZZLE_FLOOR && strlen(partial) == size);

    char first_look[] = "You are in a small square room bounded to the north and west with marble walls and to the east and south with sandstone walls.";
    map.valid = false;
    size = strlen(first_look);
    puzzle_map_update(&map, first_look, &size);
    assert(map.valid && map.tiles[1] == PUZZLE_MARBLE && map.tiles[7] == PUZZLE_SANDSTONE);

    char introduced[] = "The wall slides forward.\n"
                        "The architecture of this region is getting complex, so here's a diagram.\n"
                        "  MM = marble wall\n  ?? = unknown (blocked by walls)\n\n"
                        "      +MM MM ??+\nWest  +   .. SS+  East\n      +SS    MM+\n";
    size = strlen(introduced);
    puzzle_map_update(&map, introduced, &size);
    assert(!strcmp(introduced, "The wall slides forward.\n") && size == strlen(introduced));
    assert(map.tiles[7] == PUZZLE_FLOOR);
}

static void check_visual_save(void) {
    // A randomly described room must keep its displayed appearance through Restore,
    // even though no story variable records which description was printed.
    visual_state_reset();
    const char *description = "decorated in the Louis XIV style.\n";
    for (unsigned i = 0; description[i]; i++) visual_state_observe(1, 69, description + i, 1);
    assert(visual_state_save() == QUARTERS_LOUIS);
    uint32_t random = z_resume().random;
    assert(save_vm());
    visual_state_observe(1, 69, "complete with bunk beds", sizeof("complete with bunk beds") - 1);
    assert(visual_state_save() == QUARTERS_BUNKS);
    assert(restore_vm());
    assert(visual_state_save() == QUARTERS_LOUIS && z_resume().random == random);
    visual_state_reset();
    assert(save_vm()); // Reserved-byte zero is the previous cartridge's format.
    visual_state_observe(1, 69, "a hammock", 9);
    assert(restore_vm() && visual_state_save() == QUARTERS_UNKNOWN);
}

static void check_wrap_progress(void) {
    // A line narrower than one glyph must still advance, one glyph per row.
    const char *text = "AB C", *end;
    const char *next = wrap_line(text, 0, &end);
    assert(end == text + 1 && next == text + 1);
    next = wrap_line(next, 0, &end);
    assert(end == text + 2 && next == text + 3);
}

int main(void) {
    check_wrap_progress();
    check_transcript_codec();
    start(0);
    assert(z_global(0) == 64 && z_global(2) == 0);
    check_action_capacity(0);
    check_action(199, PRIMARY, "Give item...", "lunch", "give lunch to cyclops");
    check_action(ZORK1_TROLL, PRIMARY, "Throw item...", "knife", "throw knife at troll");
    check_action(157, PRIMARY, "Move", NULL, "move pile of leaves");
    check_action(29, PRIMARY, "Unlock with...", "skeleton key", "unlock grating with skeleton key");
    check_action(59, PRIMARY, "Cross", NULL, "cross rainbow");
    char patch_material[Z_NAME_MAX];
    story_noun(136, patch_material);
    check_action(68, PRIMARY, "Patch with...", patch_material, "plug punctured boat with viscous material");
    const uint8_t map_rooms[MAP_CELLS] = {63, 64, 65, 66, 67, 68, 69, 70, 71};
    draw_room_map(map_rooms, false, MAP_CENTER, 64);
    draw_room_map(map_rooms, true, 1, 67);
    check_dictionary((const char *const[]){"echo", "ulysse", "odysse"}, 3);
    command("open mailbox");
    assert(z_attr(230, 11));
    command("take leaflet");
    assert(z_parent(76) == 44);
    visited[8] = 1;
    trail_record(&trail, 64, 0, 77);
    trail_record(&trail, 64, 0, 78);
    assert(trail.count == 1 && trail_destination(&trail, 64, 0) == 78);
    assert(save_vm());
    command("north");
    assert(z_global(0) != 64);
    trail_record(&trail, 64, 0, z_global(0));
    trail_record(&trail, z_global(0), 2, 64);
    assert(save_vm());
    // The interrupted/corrupted newer write must leave the older bank usable.
    sram[5120 + 23] ^= 0xff;
    assert(restore_vm());
    assert(z_global(0) == 64 && z_global(2) == 2 && z_parent(76) == 44);
    assert(trail.count == 1 && trail_destination(&trail, 64, 0) == 78);
    // A damaged optional trail and a legacy save both retain story progress.
    unsigned extension = 16 + sram[8] + (sram[9] << 8);
    sram[extension + 8] ^= 1;
    assert(restore_vm() && trail.count == 0 && z_parent(76) == 44);
    memset(sram + extension, 0xff, 11);
    assert(restore_vm() && trail.count == 0 && z_parent(76) == 44);

    start(0);
    check_first_zork_dam();

    start(1);
    assert(z_global(0) == 51 && !save_exists(sram, 1));
    check_action_capacity(1);
    check_action(ZORK2_DRAGON, PRIMARY, "Kick", NULL, "kick dragon");
    check_action(229, ORDERS, "Attack...", "wizard", "demon, attack wizard");
    check_dictionary((const char *const[]){"well", "float"}, 2);
    assert(z_submit("answer \"well\""));
    const uint8_t *parse = z_dynamic() + z_resume().parse;
    assert(parse[1] == 4 && parse[4] == 6 && parse[8] == 1 && parse[12] == 4 && parse[16] == 1);
    assert(parse[10] || parse[11]);  // The word between the quotes has a dictionary entry.
    assert(z_run(1000000) == Z_PROMPT);
    command("take sword. drop sword");
    assert(z_parent(223) == 51);
    command("take lamp");
    command("save");
    command("south");
    assert(z_global(0) == 168);
    command("restore");
    assert(z_global(0) == 51 && z_parent(149) == 40);
    check_gazebo_targets();
    check_visual_save();

    start(2);
    assert(z_global(0) == 211 && !save_exists(sram, 2));
    check_action_capacity(2);
    assert(z_attr(ZORK3_VIKING_SHIP, ZORK3_SHIP_HIDDEN));
    assert(!story_cliff_man_visible() && !story_figure_defenseless());
    check_action(ZORK3_OLD_MAN, PRIMARY, "Wake", NULL, "wake man");
    check_action(ZORK3_VIKING_SHIP, PRIMARY, "Hail sailor", NULL, "hello sailor");
    check_action(22, PRIMARY, "Drink from", NULL, "drink from vial");
    check_dictionary((const char *const[]){"sailor", "dial"}, 2);
    command("take lamp");
    command("turn on lamp");
    command("south");
    assert(z_global(0) == 217 && z_global(62));
    assert(save_vm());
    command("turn off lamp");
    assert(!z_global(62));
    assert(restore_vm() && z_global(62));
    saved_map.valid = true;
    saved_map.tiles[0] = PUZZLE_MARBLE;
    saved_map.tiles[1] = PUZZLE_SANDSTONE;
    assert(save_vm());
    saved_map.valid = false;
    assert(restore_vm() && saved_map.valid && saved_map.tiles[0] == PUZZLE_MARBLE &&
           saved_map.tiles[1] == PUZZLE_SANDSTONE);
    assert(save_exists(sram, 0) && save_exists(sram, 1) && save_exists(sram, 2));
    check_puzzle_map();
    check_transcript_save();
    dump_replay_corpus();
    printf("Test replay transcript text: %u -> %u bytes (%u%% saved).\n", replay_plain_bytes, replay_encoded_bytes,
           (replay_plain_bytes - replay_encoded_bytes) * 100 / replay_plain_bytes);
    puts("All three stories, transcript compression and save round trips, object menus, puzzle maps, route history, and save recovery passed.");
    return 0;
}
