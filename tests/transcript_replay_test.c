#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "transcript_codec.h"
#include "zmachine.h"
#include "chapter.h"
#include "interaction.h"
#include "room_visuals.h"
#include "visual_state.h"

enum {
    STORY_CAPACITY = 96 * 1024,
    RESPONSE_CAPACITY = 32768,
    ENTRY_CAPACITY = 65536,
    RAW_TRANSCRIPT_CAPACITY = 128 * 1024,
    ROUTE_ENTRY_LIMIT = 1024,
    TRANSCRIPT_CAPACITY = 24 * 1024,
    QA_CHAPTERS = 3
};

typedef struct {
    unsigned encoded_size;
    unsigned plain_size;
} RetainedEntry;

static uint8_t story[STORY_CAPACITY];
static uint8_t raw_transcript[RAW_TRANSCRIPT_CAPACITY];
static unsigned raw_transcript_size;
static char response[RESPONSE_CAPACITY];
static unsigned response_size;
static unsigned chapter, seed;
static bool response_overflow;
static bool refresh_route_states;
static unsigned qa_marker_count, qa_bad_marker_bounds;
static unsigned qa_ui_focus_count;
static unsigned qa_scene_checks, qa_target_checks;
static unsigned qa_dropped_item_checks;
static unsigned qa_parser_target_checks;
static unsigned qa_parser_probe_checks, qa_room_definition_checks;
static bool qa_parser_checked[QA_CHAPTERS][512];
static bool qa_missing[QA_CHAPTERS][256][512];
static bool qa_bad_bounds[QA_CHAPTERS][256][512];

static int qa_first_marker[4];
static unsigned qa_distinct_markers;

void visual_qa_marker(int x, int y, int width, int height) {
    // A focused target gets exactly one scene box; redrawing the identical box is allowed.
    if (!qa_marker_count) {
        qa_first_marker[0] = x, qa_first_marker[1] = y, qa_first_marker[2] = width, qa_first_marker[3] = height;
        qa_distinct_markers = 1;
    } else if (x != qa_first_marker[0] || y != qa_first_marker[1] ||
               width != qa_first_marker[2] || height != qa_first_marker[3]) {
        qa_distinct_markers++;
    }
    qa_marker_count++;
    if (x < 0 || y < 15 || width <= 0 || height <= 0 || x + width > SCREEN_W || y + height > 91)
        qa_bad_marker_bounds++;
}

void visual_qa_ui_focus(void) { qa_ui_focus_count++; }

static void collect_text(const char *text, size_t size) {
    visual_state_observe(chapter, z_global(0), text, size);
    for (size_t i = 0; i < size; i++) {
        char ch = text[i] == '\r' ? '\n' : text[i];
        if (ch != '\n' && (unsigned char)ch < 32) ch = ' ';
        if (ch == '\n' && response_size >= 2 && response[response_size - 1] == '\n' &&
            response[response_size - 2] == '\n') continue;
        if (response_size + 1 >= sizeof(response)) {
            response_overflow = true;
            return;
        }
        response[response_size++] = ch;
    }
    response[response_size] = 0;
}

static void fatal(const char *reason, unsigned pc, unsigned opcode) {
    fprintf(stderr, "Story %u VM error: %s (PC %x, opcode %x)\n", chapter + 1, reason, pc, opcode);
    abort();
}

static uint32_t entropy(void) { return seed; }
static const ZPlatform platform = {collect_text, fatal, entropy, NULL, NULL};

static void response_reset(void) {
    response_size = 0;
    response_overflow = false;
    response[0] = 0;
}

static void response_trim(void) {
    static const char ending_prompt[] = "Would you like to restart the game from the beginning";
    response[response_size] = 0;
    char *prompt = strstr(response, ending_prompt);
    if (prompt) response_size = (unsigned)(prompt - response);
    while (response_size && (response[response_size - 1] == '\n' || response[response_size - 1] == ' ' ||
                             response[response_size - 1] == '>')) response_size--;
    response[response_size] = 0;
}

static void audit_parser_targets(const Targets *targets) {
    for (unsigned i = 0; i < targets->count; i++) {
        unsigned id = targets->id[i];
        char noun[Z_NAME_MAX];
        story_parser_noun(id, noun);
        if (!*noun) {
            char name[Z_NAME_MAX];
            story_name(chapter, id, name);
            fprintf(stderr, "Unaddressable target: Z%u room %u object %u (%s)\n",
                    chapter + 1, z_global(0), id, name);
        }
        assert(*noun);
        if (id < 512 && !qa_parser_checked[chapter][id]) {
            char line[COMMAND_SIZE] = "examine ", reply[96] = {0};
            assert(append_text(line, sizeof(line), noun));
            assert(z_probe(line, reply, sizeof(reply), 20000));
            if (text_has(reply, "I don't know the word")) {
                char name[Z_NAME_MAX];
                story_name(chapter, id, name);
                fprintf(stderr, "Parser rejected target noun: Z%u room %u object %u (%s): %s\n",
                        chapter + 1, z_global(0), id, name, reply);
            }
            assert(!text_has(reply, "I don't know the word"));
            qa_parser_checked[chapter][id] = true;
            qa_parser_probe_checks++;
        }
        qa_parser_target_checks++;
    }
}

static void audit_scene_target_boxes(void) {
    unsigned room_id = z_global(0);
    if (zork1_barrow_finished()) return;

    Targets targets, carried;
    story_targets(chapter, &targets, &carried);
    audit_parser_targets(&targets);
    audit_parser_targets(&carried);
    if (!targets.count) return;
    char room_name[Z_NAME_MAX];
    z_name(room_id, room_name, sizeof(room_name));
    const Color *background = story_scene(chapter, room_name);
    qa_scene_checks++;

    for (unsigned i = 0; i < targets.count; i++) {
        unsigned object = targets.id[i];
        qa_target_checks++;
        qa_marker_count = qa_bad_marker_bounds = 0;
        qa_ui_focus_count = 0;
        fill(0, 15, SCREEN_W, 76, RGB(20, 25, 24));
        if (story_dark(chapter)) {
            if (chapter == 1 && room_id == 125)
                draw_room_details(chapter, room_id, background, &targets, object);
        } else if (chapter == 2 && room_id == ROYAL_PUZZLE_ROOM) {
            PuzzleMap map = {0};
            draw_puzzle_map(&map, &targets, object);
        } else {
            draw_room_details(chapter, room_id, background, &targets, object);
        }
        draw_target_focus_box(true);
        bool scene_is_visible = !story_dark(chapter) || (chapter == 1 && room_id == 125);
        if (scene_is_visible && !qa_marker_count && !qa_missing[chapter][room_id][object]) {
            char object_name[Z_NAME_MAX];
            story_name(chapter, object, object_name);
            fprintf(stderr, "No scene box: Z%u room %u (%s) object %u (%s)\n",
                    chapter + 1, room_id, room_name, object, object_name);
            qa_missing[chapter][room_id][object] = true;
        }
        assert(qa_ui_focus_count == 1);
        if (qa_distinct_markers > 1) {
            char object_name[Z_NAME_MAX];
            story_name(chapter, object, object_name);
            fprintf(stderr, "Two scene boxes: Z%u room %u (%s) object %u (%s)\n",
                    chapter + 1, room_id, room_name, object, object_name);
        }
        assert(qa_distinct_markers <= 1);
        qa_distinct_markers = 0;
        if (qa_bad_marker_bounds && !qa_bad_bounds[chapter][room_id][object]) {
            char object_name[Z_NAME_MAX];
            story_name(chapter, object, object_name);
            fprintf(stderr, "Box outside art: Z%u room %u (%s) object %u (%s)\n",
                    chapter + 1, room_id, room_name, object, object_name);
            qa_bad_bounds[chapter][room_id][object] = true;
        }
    }
}

static void audit_dropped_item_box(const Targets *previous_carried) {
    unsigned room_id = z_global(0);
    if (story_dark(chapter)) return;

    Targets targets, carried_now;
    story_targets(chapter, &targets, &carried_now);
    unsigned selected = story_focus_dropped_item(previous_carried, room_id, &targets);
    if (selected >= targets.count) return;

    char room_name[Z_NAME_MAX];
    z_name(room_id, room_name, sizeof(room_name));
    const Color *background = story_scene(chapter, room_name);
    qa_marker_count = qa_bad_marker_bounds = 0;
    fill(0, 15, SCREEN_W, 76, RGB(20, 25, 24));
    if (chapter == 2 && room_id == ROYAL_PUZZLE_ROOM) {
        PuzzleMap map = {0};
        draw_puzzle_map(&map, &targets, targets.id[selected]);
    } else {
        draw_room_details(chapter, room_id, background, &targets, targets.id[selected]);
    }
    assert(qa_marker_count > 0);
    assert(qa_bad_marker_bounds == 0);
    qa_dropped_item_checks++;
}

static void report_target_box_gaps(void) {
    unsigned missing = 0, outside = 0;
    for (unsigned game = 0; game < QA_CHAPTERS; game++) {
        for (unsigned room = 0; room < 256; room++) {
            for (unsigned object = 1; object < 512; object++) {
                if (qa_missing[game][room][object]) {
                    missing++;
                }
                if (qa_bad_bounds[game][room][object]) {
                    outside++;
                }
            }
        }
    }
    printf("Scene target-box audit: %u states, %u focused targets, %u without a scene-specific box, "
           "%u scene boxes outside the art band; every target had at most one scene box and one yellow name-row frame.\n",
           qa_scene_checks, qa_target_checks, missing, outside);
    printf("Parser target audit: %u visible and carried target appearances had usable parser nouns.\n",
           qa_parser_target_checks);
    printf("All-room parser audit: %u room definitions checked; %u distinct target names accepted by the interpreter.\n",
           qa_room_definition_checks, qa_parser_probe_checks);
    printf("Dropped-item scene boxes checked: %u.\n", qa_dropped_item_checks);
    assert(outside == 0);
}

static unsigned encode_and_verify_entry(const char *command, unsigned command_size,
                                        const char *reply, unsigned reply_size,
                                        uint8_t *encoded, unsigned capacity, unsigned *plain_size) {
    char entry[ENTRY_CAPACITY];
    char expanded[ENTRY_CAPACITY];
    unsigned entry_size = 0, expanded_size = 0, encoded_size = 0;
    assert(command_size + reply_size <= sizeof(entry));
    if (command_size) {
        memcpy(entry, command, command_size);
        entry_size = command_size;
    }
    memcpy(entry + entry_size, reply, reply_size);
    entry_size += reply_size;
    *plain_size = entry_size;
    assert(transcript_codec_encode(entry, entry_size, encoded, capacity, &encoded_size));
    assert(transcript_codec_expand(encoded, encoded_size, expanded, sizeof(expanded), &expanded_size));
    assert(expanded_size == entry_size && !memcmp(entry, expanded, entry_size));
    return encoded_size;
}

static void account_entry(const char *command, const char *reply, unsigned reply_size,
                          unsigned *plain_total, unsigned *encoded_total,
                          RetainedEntry retained[ROUTE_ENTRY_LIMIT], unsigned *retained_head,
                          unsigned *retained_count, unsigned *retained_encoded, unsigned *retained_plain) {
    uint8_t encoded[ENTRY_CAPACITY];
    char command_line[1024];
    unsigned command_size = 0;
    if (command && *command) {
        int written = snprintf(command_line, sizeof(command_line), "> %s\n", command);
        assert(written > 0 && (unsigned)written < sizeof(command_line));
        command_size = (unsigned)written;
    }

    unsigned content_size = 0;
    unsigned encoded_size = encode_and_verify_entry(command_line, command_size, reply, reply_size,
                                                    encoded, sizeof(encoded), &content_size);
    unsigned record_size = content_size + 1;
    encoded_size++;
    assert(record_size <= sizeof(raw_transcript) - raw_transcript_size);
    if (command_size) memcpy(raw_transcript + raw_transcript_size, command_line, command_size);
    memcpy(raw_transcript + raw_transcript_size + command_size, reply, reply_size);
    raw_transcript_size += record_size - 1;
    raw_transcript[raw_transcript_size++] = 0;
    *plain_total += record_size;
    *encoded_total += encoded_size;

    while (*retained_count == ROUTE_ENTRY_LIMIT || *retained_encoded + encoded_size > TRANSCRIPT_CAPACITY) {
        RetainedEntry *oldest = &retained[*retained_head];
        *retained_encoded -= oldest->encoded_size;
        *retained_plain -= oldest->plain_size;
        *retained_head = (*retained_head + 1) % ROUTE_ENTRY_LIMIT;
        (*retained_count)--;
    }
    unsigned tail = (*retained_head + *retained_count) % ROUTE_ENTRY_LIMIT;
    retained[tail].encoded_size = encoded_size;
    retained[tail].plain_size = record_size;
    *retained_count += 1;
    *retained_encoded += encoded_size;
    *retained_plain += record_size;
}

static void dump_transcript(unsigned game) {
    const char *directory = getenv("ZORK_TRANSCRIPT_DUMP_DIR");
    if (!directory || !*directory) return;
    char path[512];
    int written = snprintf(path, sizeof(path), "%s/zork%u.bin", directory, game + 1);
    assert(written > 0 && (unsigned)written < sizeof(path));
    FILE *file = fopen(path, "wb");
    assert(file);
    assert(fwrite(raw_transcript, 1, raw_transcript_size, file) == raw_transcript_size);
    assert(!fclose(file));
}

static void poke_global(unsigned number, uint16_t value) {
    // Test-only: visit rooms the winning routes never reach by rewriting story globals.
    uint8_t *dynamic = (uint8_t *)z_dynamic();
    unsigned address = (story[0x0C] << 8 | story[0x0D]) + 2 * number;
    dynamic[address] = value >> 8;
    dynamic[address + 1] = value & 0xFF;
}

static bool object_is_room(unsigned id) {
    static const uint8_t exits[] = {31, 30, 28, 29, 27, 25, 24, 26, 23, 22, 21, 20};
    for (unsigned i = 0; i < sizeof(exits) / sizeof(*exits); i++) {
        const uint8_t *bytes;
        if (z_property(id, exits[i], &bytes)) return true;
    }
    return false;
}

static void audit_all_room_parser_targets(void) {
    enum { HERE = 0 };
    uint16_t saved_here = z_global(HERE);
    for (unsigned id = 1; id <= z_object_count(); id++) {
        if (!object_is_room(id)) continue;
        poke_global(HERE, id);
        Targets room, bag;
        story_targets(chapter, &room, &bag);
        audit_parser_targets(&room);
        audit_parser_targets(&bag);
        qa_room_definition_checks++;
    }
    poke_global(HERE, saved_here);
}

static void audit_offroute_zork2_scenes(void) {
    // The eight floor-window angled rooms and the Murky Room are off the winning route.
    extern const Color art_z2_angled_northeast[], art_z2_murky_room[];
    enum { HERE = 0, WINDOW_PASSAGE = 13, WINDOW_GLOW = 58, LIT = 52 };
    static const unsigned bases[] = {95, 142, 185, 226, 242, 75, 121, 202};
    uint16_t saved_here = z_global(HERE), saved_open = z_global(WINDOW_PASSAGE);
    uint16_t saved_glow = z_global(WINDOW_GLOW), saved_lit = z_global(LIT);
    char room_name[Z_NAME_MAX];
    poke_global(LIT, 1);
    for (unsigned state = 0; state < 6; state++) {
        poke_global(WINDOW_PASSAGE, state == 5);
        poke_global(WINDOW_GLOW, state < 5 ? state : 0);
        for (unsigned i = 0; i < sizeof(bases) / sizeof(*bases); i++) {
            poke_global(HERE, bases[i]);
            z_name(bases[i], room_name, sizeof(room_name));
            const Color *art = story_scene(1, room_name);
            if (i < 4) assert(art == art_z2_angled_northeast);
            audit_scene_target_boxes();
            // The floor window's centre pixel follows the story's six brightness states.
            extern Color visual_qa_pixel(int x, int y);
            Targets targets, carried;
            story_targets(1, &targets, &carried);
            fill(0, 15, SCREEN_W, 76, 0);
            draw_room_details(1, bases[i], art, &targets, 0);
            unsigned glow = state;
            assert(visual_qa_pixel(120, 76) == RGB(25 + glow * 32, 28 + glow * 30, 21 + glow * 20));
        }
    }
    poke_global(HERE, 237);
    z_name(237, room_name, sizeof(room_name));
    assert(story_scene(1, room_name) == art_z2_murky_room);
    audit_scene_target_boxes();
    poke_global(HERE, saved_here);
    poke_global(WINDOW_PASSAGE, saved_open);
    poke_global(WINDOW_GLOW, saved_glow);
    poke_global(LIT, saved_lit);
    printf("Off-route Zork II scenes checked: 8 window rooms in 6 window states (glow colour asserted) and the Murky Room.\n");
}

// Every object's PRIMARY list must keep headroom for "More actions..." in every state the
// winning route reaches, not only at the start of each story.
static unsigned check_action_capacity(unsigned game, unsigned peak) {
    for (unsigned id = 1; id <= z_object_count(); id++) {
        for (unsigned carried = 0; carried < 2; carried++) {
            Action actions[MAX_ACTIONS];
            unsigned count = story_actions(game, id, carried, PRIMARY, actions);
            assert(count < MAX_ACTIONS && !strcmp(actions[count - 1].name, "More actions..."));
            if (count > peak) peak = count;
        }
    }
    return peak;
}

static void run_game(unsigned game, unsigned final_room, unsigned final_score) {
    chapter = game;
    static const unsigned seeds[] = {7, 4372, 15};
    seed = seeds[game];
    char story_path[64], route_path[96], refreshed_route_path[104];
    snprintf(story_path, sizeof(story_path), "stories/zork%u.z3", game + 1);
    snprintf(route_path, sizeof(route_path), "tests/fixtures/zork%u-winning-route.tsv", game + 1);
    snprintf(refreshed_route_path, sizeof(refreshed_route_path), "%s.new", route_path);

    FILE *story_file = fopen(story_path, "rb");
    assert(story_file);
    size_t story_size = fread(story, 1, sizeof(story), story_file);
    assert(!ferror(story_file) && !fclose(story_file));
    assert(story_size > 0 && story_size < sizeof(story));

    RetainedEntry retained[ROUTE_ENTRY_LIMIT] = {{0}};
    unsigned retained_head = 0, retained_count = 0, retained_encoded = 0, retained_plain = 0;
    unsigned plain_total = 0, encoded_total = 0, commands = 0, route_drifts = 0, action_peak = 0;
    raw_transcript_size = 0;
    visual_state_reset();
    response_reset();
    z_start(story, story_size, &platform);
    assert(z_run(1000000) == Z_PROMPT && !response_overflow);
    response_trim();
    account_entry(NULL, response, response_size, &plain_total, &encoded_total,
                  retained, &retained_head, &retained_count, &retained_encoded, &retained_plain);
    audit_scene_target_boxes();
    audit_all_room_parser_targets();

    FILE *route = fopen(route_path, "r");
    assert(route);
    FILE *refreshed_route = refresh_route_states ? fopen(refreshed_route_path, "w") : NULL;
    if (refresh_route_states) {
        assert(refreshed_route);
        fputs("# Deterministic replay checkpoints: expected room and score before each command.\n", refreshed_route);
    }
    char line[1024], command[768];
    while (fgets(line, sizeof(line), route)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        unsigned expected_room, expected_score;
        int parsed = sscanf(line, "%u\t%u\t%767[^\n]", &expected_room, &expected_score, command);
        assert(parsed == 3);
        if (refresh_route_states) {
            fprintf(refreshed_route, "%u\t%u\t%s\n", z_global(0), z_global(1), command);
        } else if (z_global(0) != expected_room || z_global(1) != expected_score) {
            route_drifts++;
        }
        Targets prior_room, previous_carried;
        story_targets(chapter, &prior_room, &previous_carried);
        response_reset();
        assert(z_submit(command));
        ZResult result = z_run(1000000);
        assert((result == Z_PROMPT || result == Z_ENDED) && !response_overflow);
        response_trim();
        account_entry(command, response, response_size, &plain_total, &encoded_total,
                      retained, &retained_head, &retained_count, &retained_encoded, &retained_plain);
        audit_dropped_item_box(&previous_carried);
        audit_scene_target_boxes();
        if (result == Z_PROMPT) action_peak = check_action_capacity(game, action_peak);
        commands++;
    }
    assert(!ferror(route) && !fclose(route));
    assert(commands > 100);
    assert(z_global(0) == final_room && z_global(1) == final_score);
    if (game == 1) audit_offroute_zork2_scenes();
    if (refresh_route_states) {
        assert(!ferror(refreshed_route) && !fclose(refreshed_route));
        assert(rename(refreshed_route_path, route_path) == 0);
    }
    dump_transcript(game);

    unsigned saved = plain_total ? (plain_total - encoded_total) * 100 / plain_total : 0;
    assert(saved >= 40);
    assert(route_drifts == 0);
    printf("Zork %u full route: %u commands / %u moves, %u transcript entries, %u -> %u bytes (%u%% saved), %u reference-state drifts.\n",
           game + 1, commands, z_global(2), commands + 1, plain_total, encoded_total, saved, route_drifts);
    printf("  Primary action peak along the route: %u of %u.\n", action_peak, MAX_ACTIONS);
    printf("  24 KiB runtime log retains %u of %u plain bytes across %u entries (%u compressed bytes).\n",
           retained_plain, plain_total, retained_count, retained_encoded);
}

int main(int argc, char **argv) {
    assert(argc == 1 || (argc == 2 && !strcmp(argv[1], "--refresh-route-states")));
    refresh_route_states = argc == 2;
    run_game(0, 244, 350);
    run_game(1, 230, 400);
    run_game(2, 46, 7);
    report_target_box_gaps();
    puts("All full-route transcript entries decoded byte-for-byte and reached their expected endings.");
    return 0;
}
