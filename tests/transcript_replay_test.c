#include <assert.h>
#include <stdint.h>
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
    REPLAY_REPLY_SIZE = 512,
    STORY_CAPACITY = 96 * 1024,
    RESPONSE_CAPACITY = 32768,
    ENTRY_CAPACITY = 65536,
    RAW_TRANSCRIPT_CAPACITY = 128 * 1024,
    ROUTE_ENTRY_LIMIT = 1024,
    TRANSCRIPT_CAPACITY = 24 * 1024,
    QA_CHAPTERS = 3
};

enum { QA_BOAT, QA_BUCKET, QA_BASKET, QA_SPECIAL_VEHICLE, QA_VEHICLE_TYPES };

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
static unsigned qa_scene_checks, qa_target_checks;
static unsigned qa_dropped_item_checks;
static unsigned qa_target_visibility_appearances, qa_hidden_target_rejections;
static unsigned qa_parser_target_checks;
static unsigned qa_parser_probe_checks, qa_room_definition_checks;
static bool qa_parser_checked[QA_CHAPTERS][512];
static bool qa_missing[QA_CHAPTERS][256][512];
static bool qa_bad_bounds[QA_CHAPTERS][256][512];
static bool qa_hidden_target_reported[QA_CHAPTERS][256][512];
static bool qa_action_menu_checked[QA_CHAPTERS][256][512];
static bool qa_carried_action_menu_checked[QA_CHAPTERS][512];
static unsigned qa_action_menu_checks, qa_filtered_default_actions, qa_action_command_checks;
static unsigned qa_max_action_menu_effect_probes;
static unsigned qa_total_masked_probes, qa_max_masked_probes, qa_masked_menu_checks;
static unsigned qa_total_reference_probes, qa_max_reference_probes;
static unsigned qa_fast_proposed_actions, qa_fast_rejected_actions, qa_fast_missing_actions;
typedef struct {
    uint16_t room, object, parent;
    uint8_t game, carried;
    uint32_t attributes, actions;
} QAActionProfile;
static QAActionProfile qa_action_profiles[1024];
static unsigned qa_action_profile_count, qa_action_profile_conflicts;
static FILE *qa_action_profile_output;
static bool qa_enter_exit_states[QA_CHAPTERS][QA_VEHICLE_TYPES][2];
static unsigned qa_enter_exit_menu_checks;
static bool qa_opening_family_checked[QA_CHAPTERS][256][512][2];
static bool qa_climb_family_checked[QA_CHAPTERS][256][512][2];
static unsigned qa_opening_family_checks, qa_climb_family_checks;

static int qa_first_marker[4];
static int qa_conflicting_marker[4];
static unsigned qa_distinct_markers;

static bool action_reply_rejects(Action action, unsigned object);
static bool action_list_has(const Action *actions, unsigned count, const char *name);

static uint32_t qa_profile_actions_hash(const Action *actions, unsigned count) {
    uint32_t hash = 2166136261u;
    for (unsigned i = 0; i < count; i++) {
        const char *parts[] = {actions[i].name, actions[i].verb, actions[i].argument};
        for (unsigned part = 0; part < sizeof(parts) / sizeof(parts[0]); part++) {
            const char *p = parts[part];
            while (p && *p) hash = (hash ^ (uint8_t)*p++) * 16777619u;
            hash = (hash ^ 0xffu) * 16777619u;
        }
        hash = (hash ^ actions[i].kind) * 16777619u;
    }
    return hash;
}

static bool qa_action_equal(Action left, Action right) {
    const char *a[] = {left.name, left.verb, left.argument};
    const char *b[] = {right.name, right.verb, right.argument};
    for (unsigned i = 0; i < sizeof(a) / sizeof(a[0]); i++) {
        if ((!a[i]) != (!b[i])) return false;
        if (a[i] && strcmp(a[i], b[i])) return false;
    }
    return left.kind == right.kind;
}

static void qa_dump_action_profile(unsigned room_id, unsigned object, bool carried,
                                   const Action *actions, unsigned count) {
    if (!qa_action_profile_output) return;
    uint32_t attributes = 0;
    for (unsigned i = 0; i < 32; i++) if (z_attr(object, i)) attributes |= 1u << i;
    fprintf(qa_action_profile_output, "%u\t%u\t%u\t%u\t%u\t%08x\t%u",
            chapter, room_id, object, z_parent(object), carried, attributes, count);
    for (unsigned i = 0; i < count; i++)
        fprintf(qa_action_profile_output, "\t%s\t%s\t%s\t%u",
                actions[i].name ? actions[i].name : "-",
                actions[i].verb ? actions[i].verb : "-",
                actions[i].argument ? actions[i].argument : "-",
                (unsigned)actions[i].kind);
    fputc('\n', qa_action_profile_output);
}

static void qa_check_masked_menu(unsigned room_id, unsigned object, bool carried,
                                 const Action *reference, unsigned reference_count) {
    Action masked[MAX_ACTIONS];
    story_menu_probe_begin();
    story_menu_probe_set_budget(STORY_MENU_PROBE_BUDGET);
    unsigned candidate_count = story_actions(chapter, object, carried, PRIMARY, masked);
    unsigned count = story_filter_defaults_masked(chapter, object, masked, candidate_count);
    unsigned probes = story_menu_probes_used();
    unsigned effects = story_menu_effect_probes_used();
    story_menu_probe_end();
    qa_total_masked_probes += probes;
    qa_masked_menu_checks++;
    if (probes > qa_max_masked_probes) qa_max_masked_probes = probes;
    if (effects > qa_max_action_menu_effect_probes) qa_max_action_menu_effect_probes = effects;
    if (count != reference_count) {
        fprintf(stderr, "masked menu count mismatch game %u room %u object %u: %u vs %u\n",
                chapter, room_id, object, reference_count, count);
    }
    bool equal = count == reference_count;
    for (unsigned i = 0; equal && i < count; i++) equal = qa_action_equal(reference[i], masked[i]);
    if (!equal && !qa_action_profile_output) {
        for (unsigned i = 0; i < reference_count; i++)
            fprintf(stderr, " ref %u %s / %s\n", i, reference[i].name,
                    reference[i].verb ? reference[i].verb : "-");
        for (unsigned i = 0; i < count; i++)
            fprintf(stderr, " got %u %s / %s\n", i, masked[i].name,
                    masked[i].verb ? masked[i].verb : "-");
        assert(false);
    }
}

void visual_qa_marker(int x, int y, int width, int height) {
    // A focused target gets exactly one scene box; redrawing the identical box is allowed.
    if (!qa_marker_count) {
        qa_first_marker[0] = x, qa_first_marker[1] = y, qa_first_marker[2] = width, qa_first_marker[3] = height;
        qa_distinct_markers = 1;
    } else if (x != qa_first_marker[0] || y != qa_first_marker[1] ||
               width != qa_first_marker[2] || height != qa_first_marker[3]) {
        qa_conflicting_marker[0] = x, qa_conflicting_marker[1] = y;
        qa_conflicting_marker[2] = width, qa_conflicting_marker[3] = height;
        qa_distinct_markers++;
    }
    qa_marker_count++;
    if (x < 0 || y < 15 || width <= 0 || height <= 0 || x + width > SCREEN_W || y + height > 91)
        qa_bad_marker_bounds++;
}

void visual_qa_abort(void) { abort(); }

// The scene renderer reports the focused visual independently of the box. A
// visual is either a drawn sprite (kind 0) or a named scene feature (kind 1).
static unsigned qa_box_object, qa_visual_object, qa_visual_kind;
static int qa_box_rect[4], qa_visual_rect[4];
static bool qa_box_seen, qa_visual_seen;
static unsigned qa_reported, qa_visual_reports, qa_sprite_visual_reports, qa_feature_visual_reports;
extern Color visual_qa_pixel(int x, int y);

typedef struct {
    unsigned chapter, room, object;
    uint64_t image_hash;
} QAFocusFrameKey;

static QAFocusFrameKey qa_focus_frame_keys[8192];
static unsigned qa_focus_frame_key_count, qa_focus_frame_count;
static FILE *qa_focus_frame_manifest;

// Optional evidence export for reviewing hotspots on art baked into a room
// background. The capture contains the actual rendered art band, including the
// current focus box. Set ZORK_FOCUS_REVIEW_DIR to an existing output directory.
static void qa_capture_focus_frame(unsigned chapter_id, unsigned room, unsigned object) {
    const char *directory = getenv("ZORK_FOCUS_REVIEW_DIR");
    if (!directory || !*directory || qa_visual_kind != 1) return;

    uint64_t hash = 1469598103934665603ull;
    for (int y = 15; y < 91; y++) {
        for (int x = 0; x < SCREEN_W; x++) {
            uint16_t pixel = visual_qa_pixel(x, y);
            hash = (hash ^ (pixel & 0xffu)) * 1099511628211ull;
            hash = (hash ^ (pixel >> 8)) * 1099511628211ull;
        }
    }
    for (unsigned i = 0; i < qa_focus_frame_key_count; i++) {
        QAFocusFrameKey *key = &qa_focus_frame_keys[i];
        if (key->chapter == chapter_id && key->room == room && key->object == object &&
            key->image_hash == hash) return;
    }
    if (qa_focus_frame_key_count >= sizeof(qa_focus_frame_keys) / sizeof(qa_focus_frame_keys[0])) {
        fprintf(stderr, "Focus review capture limit exceeded.\n");
        abort();
    }
    qa_focus_frame_keys[qa_focus_frame_key_count++] =
        (QAFocusFrameKey){chapter_id, room, object, hash};

    if (!qa_focus_frame_manifest) {
        char manifest_path[1024];
        int length = snprintf(manifest_path, sizeof(manifest_path), "%s/frames.tsv", directory);
        if (length < 0 || (size_t)length >= sizeof(manifest_path)) abort();
        qa_focus_frame_manifest = fopen(manifest_path, "w");
        if (!qa_focus_frame_manifest) {
            perror(manifest_path);
            abort();
        }
        fputs("chapter\troom\troom_name\tobject\ttarget_name\timage_hash\tbox_x\tbox_y\tbox_w\tbox_h\tfile\n",
              qa_focus_frame_manifest);
    }

    char image_path[1024], image_name[96];
    snprintf(image_name, sizeof(image_name), "z%u-r%03u-o%03u-%016llx.ppm",
             chapter_id + 1, room, object, (unsigned long long)hash);
    int length = snprintf(image_path, sizeof(image_path), "%s/%s", directory, image_name);
    if (length < 0 || (size_t)length >= sizeof(image_path)) abort();
    FILE *image = fopen(image_path, "wb");
    if (!image) {
        perror(image_path);
        abort();
    }
    fprintf(image, "P6\n%u 76\n255\n", SCREEN_W);
    for (int y = 15; y < 91; y++) {
        for (int x = 0; x < SCREEN_W; x++) {
            uint16_t pixel = visual_qa_pixel(x, y);
            unsigned red = pixel & 31u, green = (pixel >> 5) & 31u, blue = (pixel >> 10) & 31u;
            uint8_t rgb[3] = {(uint8_t)(red * 255u / 31u), (uint8_t)(green * 255u / 31u),
                              (uint8_t)(blue * 255u / 31u)};
            fwrite(rgb, sizeof(rgb), 1, image);
        }
    }
    fclose(image);
    char room_name[Z_NAME_MAX], target_name[Z_NAME_MAX];
    z_name(room, room_name, sizeof(room_name));
    story_name(chapter_id, object, target_name);
    fprintf(qa_focus_frame_manifest, "%u\t%u\t%s\t%u\t%s\t%016llx\t%d\t%d\t%d\t%d\t%s\n",
            chapter_id + 1, room, room_name, object, target_name, (unsigned long long)hash,
            qa_box_rect[0], qa_box_rect[1] - 15, qa_box_rect[2], qa_box_rect[3], image_name);
    fflush(qa_focus_frame_manifest);
    qa_focus_frame_count++;
}

void visual_qa_focused_box(unsigned object, int x, int y, int width, int height) {
    if (qa_box_seen) return; // first box of the frame
    qa_reported++;
    qa_box_seen = true;
    qa_box_object = object;
    qa_box_rect[0] = x; qa_box_rect[1] = y; qa_box_rect[2] = width; qa_box_rect[3] = height;
}

void visual_qa_focused_visual(unsigned object, int kind, int x, int y, int width, int height) {
    if (qa_visual_seen) return; // first visual representation of the frame
    qa_visual_reports++;
    if (kind == 0) qa_sprite_visual_reports++;
    else qa_feature_visual_reports++;
    qa_visual_seen = true;
    qa_visual_object = object;
    qa_visual_kind = (unsigned)kind;
    qa_visual_rect[0] = x; qa_visual_rect[1] = y; qa_visual_rect[2] = width; qa_visual_rect[3] = height;
}

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
    if (z_probe_last_line()) fprintf(stderr, "during probe: %s\n", z_probe_last_line());
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

static bool target_examine_says_absent(const char *reply) {
    return text_has(reply, "You can't see any") || text_has(reply, "I don't see any") ||
           text_has(reply, "You're not at") || text_has(reply, "You are not at") ||
           text_has(reply, "There is no ") || text_has(reply, "isn't here") ||
           text_has(reply, "is not here");
}

// The Master outside a shut cell door is addressed through the barred window;
// "examine" says he is not in the cell, but the story still accepts orders to
// him from inside, so he stays a legitimate target.
static bool target_is_outside_cell(unsigned room, unsigned object) {
    return chapter == 2 && object == 11 && (room == 45 || room == 204 || room == 225);
}

static void audit_room_target_visibility(const Targets *targets) {
    unsigned room = z_global(0);
    if (chapter == 2 && room == 160)
        for (unsigned i = 0; i < targets->count; i++) assert(targets->id[i] != 152);
    for (unsigned i = 0; i < targets->count; i++) {
        unsigned object = targets->id[i];
        if (object >= TARGET_SELF || object >= 512) continue;
        qa_target_visibility_appearances++;
        char noun[Z_NAME_MAX], line[COMMAND_SIZE] = "examine ", reply[192] = {0};
        story_parser_noun(object, noun);
        if (!*noun || !append_text(line, sizeof(line), noun) ||
            !z_probe(line, reply, sizeof(reply), 20000)) continue;
        if (!target_examine_says_absent(reply) || qa_hidden_target_reported[chapter][room][object]) continue;
        if (target_is_outside_cell(room, object)) continue;
        char room_name[Z_NAME_MAX], object_name[Z_NAME_MAX];
        z_name(room, room_name, sizeof(room_name));
        story_name(chapter, object, object_name);
        fprintf(stderr, "Target may be hidden: Z%u room %u (%s), object %u (%s): %s\n",
                chapter + 1, room, room_name, object, object_name, reply);
        qa_hidden_target_reported[chapter][room][object] = true;
        qa_hidden_target_rejections++;
    }
}

static void normalize_full_action_reply(const char *reply, char *out, unsigned capacity) {
    unsigned used = 0;
    const char *p = reply;
    while (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t') p++;
    if (*p == '(') {
        while (*p && *p != ')') p++;
        if (*p == ')') p++;
        while (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t') p++;
    }
    bool pending_space = false;
    while (*p) {
        char ch = *p++;
        if (ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t') {
            pending_space = used != 0;
            continue;
        }
        if (pending_space && used + 1 < capacity) out[used++] = ' ';
        pending_space = false;
        if (ch >= 'A' && ch <= 'Z') ch = (char)(ch - 'A' + 'a');
        if (used + 1 < capacity) out[used++] = ch;
    }
    out[used] = 0;
}

static void audit_default_action_menu(unsigned room_id, unsigned object, bool carried) {
    bool *checked = carried ? &qa_carried_action_menu_checked[chapter][object]
                            : &qa_action_menu_checked[chapter][room_id][object];
    if (*checked) return;
    *checked = true;

    Action actions[MAX_ACTIONS], proposed[MAX_ACTIONS];
    story_menu_probe_begin();
    unsigned before = story_actions(chapter, object, carried, PRIMARY, actions);
    memcpy(proposed, actions, before * sizeof(*actions));
    unsigned count = story_filter_defaults(object, actions, before);
    unsigned effect_probes = story_menu_effect_probes_used();
    unsigned reference_probes = story_menu_probes_used();
    qa_total_reference_probes += reference_probes;
    if (reference_probes > qa_max_reference_probes) qa_max_reference_probes = reference_probes;
    if (effect_probes > qa_max_action_menu_effect_probes)
        qa_max_action_menu_effect_probes = effect_probes;
    story_menu_probe_end();
    assert(effect_probes <= 24);
    assert(count > 0);
    assert(count <= MAX_ACTIONS - 1);
    qa_check_masked_menu(room_id, object, carried, actions, count);
    assert(reference_probes <= 256);
    qa_action_menu_checks++;
    if (before > count) qa_filtered_default_actions += before - count;

    for (unsigned i = 0; i < before; i++) {
        if (proposed[i].kind != DO_OBJECT && proposed[i].kind != DO_LITERAL) continue;
        qa_fast_proposed_actions++;
        if (action_reply_rejects(proposed[i], object)) qa_fast_rejected_actions++;
    }
    for (unsigned i = 0; i < count; i++) {
        if (actions[i].kind != DO_OBJECT && actions[i].kind != DO_LITERAL) continue;
        if (!action_list_has(proposed, before, actions[i].name)) qa_fast_missing_actions++;
    }

    uint32_t attributes = 0;
    for (unsigned i = 0; i < 32; i++) if (z_attr(object, i)) attributes |= 1u << i;
    QAActionProfile profile = {
        (uint16_t)room_id, (uint16_t)object, z_parent(object), (uint8_t)chapter,
        (uint8_t)carried, attributes, qa_profile_actions_hash(actions, count)
    };
    bool profile_found = false;
    for (unsigned i = 0; i < qa_action_profile_count; i++) {
        QAActionProfile old = qa_action_profiles[i];
        if (old.game == profile.game && old.room == profile.room && old.object == profile.object &&
            old.parent == profile.parent && old.carried == profile.carried &&
            old.attributes == profile.attributes) {
            profile_found = true;
            if (old.actions != profile.actions) qa_action_profile_conflicts++;
            break;
        }
    }
    if (!profile_found) {
        assert(qa_action_profile_count < sizeof(qa_action_profiles) / sizeof(qa_action_profiles[0]));
        qa_action_profiles[qa_action_profile_count++] = profile;
    }
    qa_dump_action_profile(room_id, object, carried, actions, count);

    bool has_menu = count && actions[count - 1].kind == SHOW_CATEGORIES;
    if (has_menu)
        for (unsigned i = 0; i + 1 < count; i++) assert(actions[i].kind != SHOW_CATEGORIES);

    char seen[MAX_ACTIONS][REPLAY_REPLY_SIZE];
    ZProbeEffect seen_effects[MAX_ACTIONS];
    const char *seen_names[MAX_ACTIONS];
    unsigned seen_count = 0;
    bool has_examine = false;
    for (unsigned i = 0; i < count; i++) {
        if (actions[i].verb && text_equal(actions[i].verb, "examine")) has_examine = true;
        if (actions[i].kind != DO_OBJECT && actions[i].kind != DO_LITERAL) continue;
        char line[COMMAND_SIZE], reply[REPLAY_REPLY_SIZE] = {0};
        ZProbeEffect effect = {0, 0, false};
        assert(action_command(actions[i], object, NULL, line));
        assert(z_probe_with_effect(line, reply, sizeof(reply), 20000, &effect));
        assert(effect.valid);
        if (text_has(reply, "I don't know the word") ||
            text_has(reply, "I don't understand that sentence") ||
            text_has(reply, "That sentence isn't one I recognize") ||
            text_has(reply, "That sentence is not one I recognize")) {
            char name[Z_NAME_MAX], noun[Z_NAME_MAX];
            story_name(chapter, object, name);
            story_parser_noun(object, noun);
            fprintf(stderr, "Parser rejected default in Z%u room %u object %u (%s, parent %u, noun '%s'): %s (%s) -> %s\n",
                    chapter + 1, room_id, object, name, z_parent(object), noun, actions[i].name, line, reply);
        }
        assert(!text_has(reply, "I don't know the word"));
        assert(!text_has(reply, "I don't understand that sentence"));
        assert(!text_has(reply, "That sentence isn't one I recognize"));
        assert(!text_has(reply, "That sentence is not one I recognize"));
        char norm[48] = {0}, full_reply[REPLAY_REPLY_SIZE] = {0};
        action_reply_normalized(reply, norm, sizeof(norm));
        normalize_full_action_reply(reply, full_reply, sizeof(full_reply));
        // Deduplication uses the full reply and VM effect. A repeated first
        // sentence can still lead to different details or a distinct result.
        // "Not available here" replies are not results and may repeat.
        if (norm[0] && !text_has(norm, "you can't see any") && !text_has(norm, "you're not at")) {
            for (unsigned j = 0; j < seen_count; j++) {
                if (!strcmp(seen[j], full_reply) &&
                    seen_effects[j].first == effect.first && seen_effects[j].second == effect.second) {
                    fprintf(stderr, "duplicate full result in room %u from %s object %u (already offered as %s): %s\n",
                            room_id, actions[i].name, object, seen_names[j], full_reply);
                    assert(false);
                }
            }
            unsigned k = 0;
            do { seen[seen_count][k] = full_reply[k]; } while (full_reply[k++]);
            seen_effects[seen_count] = effect;
            seen_names[seen_count] = actions[i].name;
            seen_count++;
        }
        qa_action_command_checks++;
    }
    assert(has_examine);
}

static void audit_enter_exit_menu(unsigned object) {
    char name[Z_NAME_MAX];
    story_noun(object, name);
    unsigned type;
    if (chapter == 2 && object == 122) type = QA_SPECIAL_VEHICLE;
    else if (text_has(name, "boat")) type = QA_BOAT;
    else if (text_has(name, "bucket")) type = QA_BUCKET;
    else if (text_has(name, "basket")) type = QA_BASKET;
    else return;

    bool inside = z_parent(chapters[chapter].player) == object;
    const char *expected = inside ?
        ((type == QA_BUCKET || type == QA_BASKET) ? "Stand up" : "Get out") : "Enter";
    const char *opposite = inside ? "Enter" :
        ((type == QA_BUCKET || type == QA_BASKET) ? "Stand up" : "Get out");
    Action candidates[MAX_ACTIONS];
    unsigned candidate_count = story_actions(chapter, object, false, PRIMARY, candidates);
    Action actions[MAX_ACTIONS];
    unsigned count = story_actions(chapter, object, false, PRIMARY, actions);
    count = story_filter_defaults(object, actions, count);
    bool found_expected = false, found_opposite = false;
    for (unsigned i = 0; i < count; i++) {
        if (strcmp(actions[i].name, expected) == 0) found_expected = true;
        if (strcmp(actions[i].name, opposite) == 0) found_opposite = true;
    }
    // The filter drops actions the story refuses or answers with no effect
    // (a closed basket cannot be entered), so judge against viability.
    bool expected_viable = false, opposite_viable = false;
    for (unsigned i = 0; i < candidate_count; i++) {
        if (strcmp(candidates[i].name, expected) == 0)
            expected_viable = !action_reply_rejects(candidates[i], object);
        if (strcmp(candidates[i].name, opposite) == 0)
            opposite_viable = !action_reply_rejects(candidates[i], object);
    }
    // The expected action may be legitimately absent when its result duplicates
    // another offered action (dedup keeps the earlier one).
    bool expected_ok = found_expected == expected_viable;
    if (!expected_ok && !found_expected && expected_viable) {
        char expected_norm[48] = {0};
        for (unsigned i = 0; i < candidate_count; i++) {
            if (strcmp(candidates[i].name, expected) != 0) continue;
            char line[COMMAND_SIZE], reply[REPLAY_REPLY_SIZE] = {0};
            if (action_command(candidates[i], object, NULL, line) &&
                z_probe(line, reply, sizeof(reply), 20000))
                action_reply_normalized(reply, expected_norm, sizeof(expected_norm));
            break;
        }
        if (expected_norm[0]) {
            for (unsigned i = 0; i < count && !expected_ok; i++) {
                char line[COMMAND_SIZE], reply[REPLAY_REPLY_SIZE] = {0}, norm[48] = {0};
                if (actions[i].kind == DO_OBJECT && action_command(actions[i], object, NULL, line) &&
                    z_probe(line, reply, sizeof(reply), 20000)) {
                    action_reply_normalized(reply, norm, sizeof(norm));
                    if (norm[0] && !strcmp(norm, expected_norm)) expected_ok = true;
                }
            }
        }
    }
    if (!expected_ok || (found_opposite && !opposite_viable)) {
        fprintf(stderr, "enter/exit audit: chapter %u object %u inside %d expected %s viable %d found %d opposite %s viable %d found %d\n",
                chapter, object, inside, expected, expected_viable, found_expected, opposite, opposite_viable, found_opposite);
        for (unsigned i = 0; i < count; i++) {
            char line[COMMAND_SIZE], reply[REPLAY_REPLY_SIZE] = {0}, norm[48] = {0};
            fprintf(stderr, "  [%s]", actions[i].name);
            if (actions[i].kind == DO_OBJECT && action_command(actions[i], object, NULL, line) &&
                z_probe(line, reply, sizeof(reply), 20000)) {
                action_reply_normalized(reply, norm, sizeof(norm));
                fprintf(stderr, "='%s'", norm);
            }
        }
        fprintf(stderr, "\n");
    }
    assert(expected_ok && !(found_opposite && !opposite_viable));
    qa_enter_exit_states[chapter][type][inside ? 1 : 0] = true;
    qa_enter_exit_menu_checks++;
}

static bool action_list_has(const Action *actions, unsigned count, const char *name) {
    for (unsigned i = 0; i < count; i++)
        if (strcmp(actions[i].name, name) == 0) return true;
    return false;
}

static bool action_reply_rejects(Action action, unsigned object) {
    char line[COMMAND_SIZE], reply[192] = {0};
    assert(action_command(action, object, NULL, line));
    assert(z_probe(line, reply, sizeof(reply), 20000));
    bool parser_rejected = text_has(reply, "I don't know the word") ||
                           text_has(reply, "I don't understand that sentence") ||
                           text_has(reply, "That sentence isn't one I recognize") ||
                           text_has(reply, "That sentence is not one I recognize");
    char cannot[COMMAND_SIZE] = "cannot ";
    char cant[COMMAND_SIZE] = "can't ";
    bool explicitly_impossible = action.verb &&
        append_text(cannot, COMMAND_SIZE, action.verb) &&
        append_text(cant, COMMAND_SIZE, action.verb) &&
        (text_has(reply, cannot) || text_has(reply, cant));
    return parser_rejected || explicitly_impossible || action_reply_is_no_effect(reply);
}

static void assert_candidate_filter(Action *candidates, unsigned candidate_count,
                                    Action *visible, unsigned visible_count,
                                    const char *name, unsigned object) {
    for (unsigned i = 0; i < candidate_count; i++) {
        if (strcmp(candidates[i].name, name) != 0) continue;
        bool should_be_visible = !action_reply_rejects(candidates[i], object);
        bool found = action_list_has(visible, visible_count, name);
        // A viable action may still be collapsed by de-duplication when another
        // visible action produced the same reply.
        if (should_be_visible && !found) {
            char line[COMMAND_SIZE], reply[192] = {0}, expected[48] = {0};
            if (action_command(candidates[i], object, NULL, line) &&
                z_probe(line, reply, sizeof(reply), 20000))
                action_reply_normalized(reply, expected, sizeof(expected));
            for (unsigned j = 0; expected[0] && j < visible_count && !found; j++) {
                char vline[COMMAND_SIZE], vreply[192] = {0}, vnorm[48] = {0};
                if (visible[j].kind != DO_OBJECT && visible[j].kind != DO_LITERAL) continue;
                if (action_command(visible[j], object, NULL, vline) &&
                    z_probe(vline, vreply, sizeof(vreply), 20000)) {
                    action_reply_normalized(vreply, vnorm, sizeof(vnorm));
                    if (vnorm[0] && !strcmp(vnorm, expected)) found = true;
                }
            }
        }
        assert(found == should_be_visible);
        return;
    }
    assert(false);
}

static bool reply_blocks_climb_direction(const char *reply, bool upward) {
    const char *word = upward ? "upward" : "downward";
    const char *short_word = upward ? "up" : "down";
    return (text_has(reply, "doesn't lead") && text_has(reply, word)) ||
           (text_has(reply, "don't lead") && text_has(reply, word)) ||
           (text_has(reply, "does not lead") && text_has(reply, word)) ||
           (text_has(reply, "doesn't go") && text_has(reply, short_word)) ||
           (text_has(reply, "don't go") && text_has(reply, short_word)) ||
           (text_has(reply, "does not go") && text_has(reply, short_word));
}

static void audit_opening_action_family(unsigned room_id, unsigned object, bool carried) {
    char name[Z_NAME_MAX];
    story_noun(object, name);
    bool is_window = text_has(name, "window");
    bool is_door = z_attr(object, ATTR_DOOR) || text_has(name, "door");
    if (!z_attr(object, ATTR_CONTAINER) && !is_door && !is_window) return;
    if (chapter == 2 && object == ZORK3_DUNGEON_DOOR && room_id == ZORK3_DUNGEON_ENTRANCE) return;

    bool open = z_attr(object, ATTR_OPEN);
    bool *checked = &qa_opening_family_checked[chapter][room_id][object][open ? 1 : 0];
    if (*checked) return;
    *checked = true;

    Action candidates[MAX_ACTIONS], visible[MAX_ACTIONS];
    story_menu_probe_begin();
    unsigned candidate_count = story_actions(chapter, object, carried, PRIMARY, candidates);
    memcpy(visible, candidates, candidate_count * sizeof(*visible));
    unsigned visible_count = story_filter_defaults(object, visible, candidate_count);
    unsigned reference_probes = story_menu_probes_used();
    story_menu_probe_end();
    qa_total_reference_probes += reference_probes;
    if (reference_probes > qa_max_reference_probes) qa_max_reference_probes = reference_probes;
    qa_check_masked_menu(room_id, object, carried, visible, visible_count);
    qa_dump_action_profile(room_id, object, carried, visible, visible_count);
    const char *expected = open ? "Close" : "Open";
    const char *opposite = open ? "Open" : "Close";
    assert(action_list_has(candidates, candidate_count, expected));
    assert(!action_list_has(candidates, candidate_count, opposite));
    assert_candidate_filter(candidates, candidate_count, visible, visible_count, expected, object);

    bool portal = is_door || is_window;
    if (portal) {
        if (open) {
            assert(action_list_has(candidates, candidate_count, "Enter"));
            assert_candidate_filter(candidates, candidate_count, visible, visible_count, "Enter", object);
        } else assert(!action_list_has(candidates, candidate_count, "Enter"));
    }
    qa_opening_family_checks++;
}

static void audit_climb_action_family(unsigned room_id, unsigned object) {
    if (!z_attr(object, ATTR_CLIMBABLE)) return;
    bool open = z_attr(object, ATTR_OPEN);
    bool *checked = &qa_climb_family_checked[chapter][room_id][object][open ? 1 : 0];
    if (*checked) return;
    *checked = true;

    Action actions[MAX_ACTIONS];
    story_menu_probe_begin();
    unsigned count = story_actions(chapter, object, false, PRIMARY, actions);
    count = story_filter_defaults(object, actions, count);
    unsigned reference_probes = story_menu_probes_used();
    story_menu_probe_end();
    qa_total_reference_probes += reference_probes;
    if (reference_probes > qa_max_reference_probes) qa_max_reference_probes = reference_probes;
    qa_check_masked_menu(room_id, object, false, actions, count);
    qa_dump_action_profile(room_id, object, false, actions, count);
    const Action *climb = NULL;
    for (unsigned i = 0; i < count; i++) {
        if (strcmp(actions[i].name, "Climb") == 0 || strcmp(actions[i].name, "Climb up") == 0 ||
            strcmp(actions[i].name, "Climb down") == 0 || strcmp(actions[i].name, "Go up") == 0 ||
            strcmp(actions[i].name, "Go down") == 0) {
            climb = &actions[i];
            break;
        }
    }
    if (!climb) return; // The story may explicitly reject climbing this target.

    char line[COMMAND_SIZE], reply[192] = {0};
    assert(action_command(*climb, object, NULL, line));
    assert(z_probe(line, reply, sizeof(reply), 20000));
    assert(!text_has(reply, "I don't know the word"));
    assert(!text_has(reply, "I don't understand that sentence"));
    assert(!text_has(reply, "That sentence isn't one I recognize"));
    assert(!text_has(reply, "That sentence is not one I recognize"));
    if (strcmp(climb->name, "Climb up") == 0 || strcmp(climb->name, "Go up") == 0)
        assert(!reply_blocks_climb_direction(reply, true));
    else if (strcmp(climb->name, "Climb down") == 0 || strcmp(climb->name, "Go down") == 0)
        assert(!reply_blocks_climb_direction(reply, false));
    else if (reply_blocks_climb_direction(reply, true) || reply_blocks_climb_direction(reply, false)) {
        fprintf(stderr, "climb audit: room %u object %u action %s reply '%s'\n",
                room_id, object, climb->name, reply);
        assert(false);
    }
    qa_climb_family_checks++;
}

// Name-row only: the story keeps these off the room view (carried, in a
// container, or no scene art), so a scene box would frame the wrong thing.
static bool focus_allowlisted(unsigned chapter_id, unsigned room, unsigned object) {
    if (chapter_id == 1 && room == 59 && object == 109)
        return z_attr(137, ATTR_OPEN); // The lid is only drawn while the door is shut.
    if (chapter_id == 2 && room == 54 && object == 80)
        return true; // The south wooden wall is behind the camera in the mirror-hall view.
    static const struct { uint8_t chapter; uint16_t room, object; } hidden[] = {
        {0, 82, 94}, {0, 82, 105}, {0, 82, 246}, {0, 89, 60}, {0, 117, 94},
        {0, 117, 246}, {0, 245, 60}, {0, 249, 60},
        {0, 33, 231}, {1, 233, 31}, {2, 103, 205}
    };
    for (unsigned i = 0; i < sizeof(hidden) / sizeof(hidden[0]); i++)
        if (hidden[i].chapter == chapter_id && hidden[i].room == room && hidden[i].object == object)
            return true;
    return false;
}

static unsigned qa_box_mismatch;

static void audit_scene_target_boxes(void) {
    unsigned room_id = z_global(0);
    if (zork1_barrow_finished()) return;

    Targets targets, carried;
    story_targets(chapter, &targets, &carried);
    audit_parser_targets(&targets);
    audit_parser_targets(&carried);
    audit_room_target_visibility(&targets);
    for (unsigned i = 0; i < targets.count; i++)
    {
        audit_default_action_menu(room_id, targets.id[i], false);
        audit_enter_exit_menu(targets.id[i]);
        audit_opening_action_family(room_id, targets.id[i], false);
        audit_climb_action_family(room_id, targets.id[i]);
    }
    for (unsigned i = 0; i < carried.count; i++) {
        audit_default_action_menu(room_id, carried.id[i], true);
        audit_opening_action_family(room_id, carried.id[i], true);
    }
    if (!targets.count) return;
    char room_name[Z_NAME_MAX];
    z_name(room_id, room_name, sizeof(room_name));
    qa_scene_checks++;

    for (unsigned i = 0; i < targets.count; i++) {
        unsigned object = targets.id[i];
        qa_target_checks++;
        qa_marker_count = qa_bad_marker_bounds = 0;
        qa_box_seen = qa_visual_seen = false;
        qa_box_object = qa_visual_object = 0;
        extern bool visual_qa_render_focus_frame(unsigned chapter_id, unsigned room, unsigned object);
        assert(visual_qa_render_focus_frame(chapter, room_id, object));
        if (chapter == 0 && room_id == 2 && object == ZORK1_STAIRS) {
            assert(qa_box_rect[0] >= 180); // Temple stairs are on the right, beyond the entrance.
            assert(qa_box_rect[1] >= 50);  // They descend from the right-hand floor opening.
        }
        if (chapter == 1 && room_id == 8 && object == 6)
            assert(qa_box_rect[0] + qa_box_rect[2] <= 160); // keep the well box clear of the doorway
        if (chapter == 2 && room_id == 160 && object == 107)
            assert(qa_box_rect[1] >= 75); // stone channel is on the floor, below the pole
        extern Color visual_qa_pixel(int x, int y);
        Color focus_gold = RGB(249, 192, 93);
        assert(visual_qa_pixel(2, 92) == focus_gold);
        assert(visual_qa_pixel(237, 92) == focus_gold);
        assert(visual_qa_pixel(2, 102) == focus_gold);
        assert(visual_qa_pixel(237, 102) == focus_gold);
        bool scene_is_visible = !story_dark(chapter) || (chapter == 1 && room_id == 125);
        if (scene_is_visible && !focus_allowlisted(chapter, room_id, object))
            qa_capture_focus_frame(chapter, room_id, object);
        if (scene_is_visible && !focus_allowlisted(chapter, room_id, object)) {
            // The box owner and the separately reported drawn visual must both
            // identify this target; a box can no longer label itself as proof.
            if (!qa_box_seen || qa_box_object != object) {
                fprintf(stderr, "No focus report: Z%u room %u object %u (got %u, parent %u, "
                                "%u box markers, visual %u)\n",
                        chapter + 1, room_id, object, qa_box_object, z_parent(object),
                        qa_marker_count, qa_visual_object);
                assert(false);
            }
            if (!qa_visual_seen || qa_visual_object != object) {
                fprintf(stderr, "No focused visual: Z%u room %u object %u (got %u)\n",
                        chapter + 1, room_id, object, qa_visual_object);
                assert(false);
            }
            int mx = qa_box_rect[0], my = qa_box_rect[1];
            int mw = qa_box_rect[2], mh = qa_box_rect[3];
            assert(mx == qa_first_marker[0] && my == qa_first_marker[1] &&
                   mw == qa_first_marker[2] && mh == qa_first_marker[3]);
            bool mismatch = mx > qa_visual_rect[0] + 3 || my > qa_visual_rect[1] + 3 ||
                mx + mw < qa_visual_rect[0] + qa_visual_rect[2] - 3 ||
                my + mh < qa_visual_rect[1] + qa_visual_rect[3] - 3 ||
                mw > qa_visual_rect[2] + 6 || mh > qa_visual_rect[3] + 6;
            if (mismatch) {
                fprintf(stderr, "Box mismatch: Z%u room %u object %u box %d,%d %dx%d visual %d,%d %dx%d kind %u\n",
                        chapter + 1, room_id, object, mx, my, mw, mh,
                        qa_visual_rect[0], qa_visual_rect[1], qa_visual_rect[2], qa_visual_rect[3],
                        qa_visual_kind);
                qa_box_mismatch++;
            }
        }
        if (scene_is_visible && !focus_allowlisted(chapter, room_id, object) &&
            !qa_marker_count && !qa_missing[chapter][room_id][object]) {
            char object_name[Z_NAME_MAX];
            story_name(chapter, object, object_name);
            fprintf(stderr, "No scene box: Z%u room %u (%s) object %u (%s)\n",
                    chapter + 1, room_id, room_name, object, object_name);
            qa_missing[chapter][room_id][object] = true;
        }
        if (qa_distinct_markers > 1) {
            char object_name[Z_NAME_MAX];
            story_name(chapter, object, object_name);
            fprintf(stderr, "Two scene boxes: Z%u room %u (%s) object %u (%s): "
                            "%d,%d %dx%d and %d,%d %dx%d\n",
                    chapter + 1, room_id, room_name, object, object_name,
                    qa_first_marker[0], qa_first_marker[1], qa_first_marker[2], qa_first_marker[3],
                    qa_conflicting_marker[0], qa_conflicting_marker[1],
                    qa_conflicting_marker[2], qa_conflicting_marker[3]);
        }
        assert(qa_distinct_markers <= 1);
        qa_distinct_markers = 0;
        if (qa_bad_marker_bounds && !qa_bad_bounds[chapter][room_id][object]) {
            char object_name[Z_NAME_MAX];
            story_name(chapter, object, object_name);
            fprintf(stderr, "Box outside art: Z%u room %u (%s) object %u (%s) box %d,%d %dx%d\n",
                    chapter + 1, room_id, room_name, object, object_name,
                    qa_first_marker[0], qa_first_marker[1], qa_first_marker[2], qa_first_marker[3]);
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
    qa_box_seen = qa_visual_seen = false;
    qa_box_object = qa_visual_object = 0;
    fill(0, 15, SCREEN_W, 76, RGB(20, 25, 24));
    if (chapter == 2 && room_id == ROYAL_PUZZLE_ROOM) {
        PuzzleMap map = {0};
        draw_puzzle_map(&map, &targets, targets.id[selected]);
    } else {
        draw_room_details(chapter, room_id, background, &targets, targets.id[selected]);
    }
    assert(qa_marker_count > 0);
    assert(qa_bad_marker_bounds == 0);
    assert(qa_box_seen && qa_box_object == targets.id[selected]);
    assert(qa_visual_seen && qa_visual_object == targets.id[selected]);
    int mx = qa_box_rect[0], my = qa_box_rect[1];
    int mw = qa_box_rect[2], mh = qa_box_rect[3];
    assert(mx == qa_first_marker[0] && my == qa_first_marker[1] &&
           mw == qa_first_marker[2] && mh == qa_first_marker[3]);
    assert(mx <= qa_visual_rect[0] + 3 && my <= qa_visual_rect[1] + 3);
    assert(mx + mw >= qa_visual_rect[0] + qa_visual_rect[2] - 3);
    assert(my + mh >= qa_visual_rect[1] + qa_visual_rect[3] - 3);
    assert(mw <= qa_visual_rect[2] + 6 && mh <= qa_visual_rect[3] + 6);
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
           "%u scene boxes outside the art band, %u boxes reported, %u focused visuals "
           "(%u sprites, %u scene features), %u box mismatches; "
           "every visible target had a visual report and at most one distinct box.\n",
           qa_scene_checks, qa_target_checks, missing, outside, qa_reported,
           qa_visual_reports, qa_sprite_visual_reports, qa_feature_visual_reports, qa_box_mismatch);
    assert(qa_reported > 0); // The correspondence hook must be wired up.
    assert(qa_box_mismatch == 0);
    assert(missing == 0); // Every visible room-screen target has a scene box or is allowlisted.
    printf("Parser target audit: %u visible and carried target appearances had usable parser nouns.\n",
           qa_parser_target_checks);
    printf("Room target visibility audit: %u appearances checked; %u room/object pairs were rejected by examine.\n",
           qa_target_visibility_appearances, qa_hidden_target_rejections);
    assert(qa_hidden_target_rejections == 0);
    printf("All-room parser audit: %u room definitions checked; %u distinct target names accepted by the interpreter.\n",
           qa_room_definition_checks, qa_parser_probe_checks);
    printf("Default-action audit: %u room/inventory menus checked; %u direct action commands parsed; "
           "%u parser-rejected or explicitly impossible defaults filtered.\n",
           qa_action_menu_checks, qa_action_command_checks, qa_filtered_default_actions);
    printf("No-probe rule coverage: %u proposed actions (%u rejected), %u current-menu actions missing from rules.\n",
           qa_fast_proposed_actions, qa_fast_rejected_actions, qa_fast_missing_actions);
    printf("Audited action states: %u unique room/object/attribute keys, %u conflicting repeats.\n",
           qa_action_profile_count, qa_action_profile_conflicts);
    printf("Masked action probes: %u total across %u audited states; max %u per state.\n",
           qa_total_masked_probes, qa_masked_menu_checks, qa_max_masked_probes);
    printf("Full-sweep action probes: %u total; max %u per state.\n",
           qa_total_reference_probes, qa_max_reference_probes);
    printf("Duplicate-result verification: at most %u effect probes in one menu (24-probe cap).\n",
           qa_max_action_menu_effect_probes);
    assert(qa_enter_exit_states[0][QA_BOAT][0] && qa_enter_exit_states[0][QA_BOAT][1]);
    assert(qa_enter_exit_states[1][QA_BUCKET][0] && qa_enter_exit_states[1][QA_BUCKET][1]);
    printf("Enter/exit direction audit: %u vehicle states checked; Zork I boat and Zork II bucket tested inside and outside.\n",
           qa_enter_exit_menu_checks);
    assert(qa_opening_family_checks && qa_climb_family_checks);
    printf("Action-family audit: %u open/close/portal states and %u climbable target states checked.\n",
           qa_opening_family_checks, qa_climb_family_checks);
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
    assert(argc == 1 || (argc == 2 && !strcmp(argv[1], "--refresh-route-states")) ||
           (argc == 3 && !strcmp(argv[1], "--dump-action-profiles")));
    refresh_route_states = argc == 2;
    if (argc == 3) {
        qa_action_profile_output = fopen(argv[2], "w");
        assert(qa_action_profile_output);
    }
    run_game(0, 244, 350);
    run_game(1, 230, 400);
    run_game(2, 46, 7);
    report_target_box_gaps();
    puts("All full-route transcript entries decoded byte-for-byte and reached their expected endings.");
    if (qa_action_profile_output) assert(!fclose(qa_action_profile_output));
    return 0;
}
