// Exercise the cartridge's input handlers without starting its hardware loop.
#define main trilogy_main
#include "../src/trilogy.c"
#undef main

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned route_seed;
static const char *fixture;
static unsigned fixture_line;

static _Noreturn void fail(const char *message) {
    fprintf(stderr, "%s:%u: %s\n", fixture, fixture_line, message);
    exit(EXIT_FAILURE);
}

static void require(bool condition, const char *message) {
    if (!condition) fail(message);
}

void visual_qa_abort(void) {
    fail("cartridge reported a fatal story error");
}

void visual_qa_marker(int x, int y, int width, int height) {
    (void)x; (void)y; (void)width; (void)height;
}

void visual_qa_focused_box(unsigned object, int x, int y, int width, int height) {
    (void)object;
    visual_qa_marker(x, y, width, height);
}

void visual_qa_focused_visual(unsigned object, int kind, int x, int y, int width, int height) {
    (void)kind;
    visual_qa_focused_box(object, x, y, width, height);
}

static uint32_t fixed_entropy(void) {
    return route_seed;
}

static uint16_t decode_button(char code) {
    static const char codes[] = "abud<>lrst";
    static const uint16_t buttons[] = {
        KEY_A, KEY_B, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT,
        KEY_L, KEY_R, KEY_SELECT, KEY_START
    };
    const char *button = strchr(codes, code);
    require(button != NULL, "unknown button code");
    return buttons[button - codes];
}

static void finish_turn(void) {
    for (unsigned frames = 0; busy && frames < 1000; frames++) complete_turn();
    require(!busy, "story turn did not finish");
}

static void tap_button(char code) {
    require(!busy, "a command was submitted before the last button");
    update(decode_button(code));
    require(screen != COMMAND, "route opened the keyboard");
    // Allow the action list to settle before choosing its next row.
    for (unsigned frames = 0; menu_filter_running && story_menu_filter_active() && frames < 2048; frames++)
        step_primary_menu_filter();
    require(!menu_filter_running || !story_menu_filter_active(), "action menu did not settle");
}

static void start_story(unsigned game, unsigned seed) {
    route_seed = seed;
    static uint8_t story[96 * 1024];
    static const ZPlatform platform = {story_text, fatal_error, fixed_entropy, NULL, NULL};
    char story_path[32];
    snprintf(story_path, sizeof(story_path), "stories/zork%u.z3", game + 1);
    FILE *file = fopen(story_path, "rb");
    require(file != NULL, "could not open story");
    size_t size = fread(story, 1, sizeof(story), file);
    require(!ferror(file) && feof(file), "could not read complete story");
    fclose(file);

    chapter = game;
    invalidate_primary_menu_cache();
    screen = EXPLORE;
    room_object = focus = nearby.count = carried.count = 0;
    trail.count = travel_room = 0;
    finished_prompt = successful_ending = puzzle_map.valid = false;
    memset(visited, 0, sizeof(visited));
    text_reset();
    transcript_clear();
    visual_state_reset();
    z_start(story, size, &platform);
    busy = true;
    finish_turn();
}

typedef struct {
    unsigned room;
    const char *command, *context, *current_step, *next_step;
} ReplayHintCheckpoint;

static const ReplayHintCheckpoint replay_hint_checkpoints[] = {
    {129, "read black book", NULL, "Read the black book at the Altar.", NULL},
    {238, "answer \"well\"", "Riddle from the door", "Try an answer to the riddle in the Riddle Room.", NULL},
    {ZORK3_BEAM_ROOM, "break red beam of light with sword", "Red beam of light",
     "Bring the sword to the Beam Room.", "Check the controls in the Button Room."}
};

static void check_replay_hint(const ReplayHintCheckpoint *checkpoint, bool before);

static void run_route(unsigned game, const char *route, unsigned seed,
                      unsigned final_room, unsigned final_score,
                      const char *alternate_command, FILE *summary) {
    fixture = route;
    fixture_line = 0;
    start_story(game, seed);

    FILE *file = fopen(route, "r");
    require(file != NULL, "could not open button fixture");
    unsigned turns = 0;
    bool read_plaque = false;
    bool used_alternate = false;
    bool checked_hints = false;
    char line[512];
    while (fgets(line, sizeof(line), file)) {
        fixture_line++;
        if (line[0] == '#' || line[0] == '\n') continue;
        unsigned expected_room, expected_score;
        char keys[256], expected_command[COMMAND_SIZE];
        require(sscanf(line, "%u\t%u\t%255s\t%127[^\r\n]",
                       &expected_room, &expected_score, keys, expected_command) == 4,
                "invalid button fixture row");
        const ReplayHintCheckpoint *hint = &replay_hint_checkpoints[game];
        bool inspect_hints = !checked_hints && room_object == hint->room &&
                             expected_room == hint->room && !strcmp(expected_command, hint->command);
        if (inspect_hints) check_replay_hint(hint, true);
        for (const char *key = keys; *key; key++) tap_button(*key);
        require(busy, "buttons did not submit a command");
        if (strcmp(pending_command, expected_command)) {
            fprintf(stderr, "%s:%u: expected command '%s', got '%s'\n",
                    fixture, fixture_line, expected_command, pending_command);
            exit(EXIT_FAILURE);
        }
        finish_turn();
        if (z_global(0) != expected_room || z_global(1) != expected_score) {
            fprintf(stderr, "%s:%u: after '%s', expected room %u and score %u, got %u and %u\n",
                    fixture, fixture_line, expected_command, expected_room, expected_score,
                    z_global(0), z_global(1));
            exit(EXIT_FAILURE);
        }
        if (!strcmp(expected_command, "read plaque")) {
            require(text_has(response, "The plaque explains"), "plaque inscription did not appear");
            read_plaque = true;
        }
        if (alternate_command && !strcmp(expected_command, alternate_command)) used_alternate = true;
        if (inspect_hints) {
            check_replay_hint(hint, false);
            checked_hints = true;
        }
        turns++;
    }
    require(!ferror(file), "could not read complete button fixture");
    fclose(file);
    require(z_global(0) == final_room && z_global(1) == final_score, "ending was not reached");
    require(!alternate_command || used_alternate, "replay did not use the required alternate action");
    require(checked_hints, "winning replay did not reach its before-and-after hint check");
    if (game == 2) {
        require(successful_ending, "Zork III successful ending did not appear");
        require(read_plaque, "Zork III route did not read the plaque");
    }
    fprintf(summary, "Zork %u %sbutton replay passed (%u commands, score %u).\n",
            game + 1, alternate_command ? "alternate " : "", turns, final_score);
}

static void expect_screen(Screen expected) {
    require(screen == expected, "unexpected screen after button press");
}

static void select_row(unsigned row) {
    require(row < list_count(), "menu row is unavailable");
    for (unsigned i = 0; selected != row && i < list_count(); i++) tap_button('d');
    require(selected == row, "menu row could not be reached");
}

static void browse_rows(void) {
    unsigned count = list_count();
    if (!count) return;
    unsigned original_row = selected;
    Screen original_screen = screen;
    tap_button('u');
    require(selected == (original_row + count - 1) % count, "Up did not move to the previous row");
    for (unsigned i = 0; i < count; i++) {
        unsigned row = selected;
        tap_button('d');
        require(selected == (row + 1) % count, "Down did not reach the next row");
    }
    tap_button('d');
    require(selected == original_row, "menu did not restore its selection after browsing");
    if (count > MENU_LINES) {
        tap_button('r');
        unsigned next = original_row + MENU_LINES < count ? original_row + MENU_LINES : count - 1;
        require(selected == next, "R did not advance the menu page");
        tap_button('l');
        require(selected == (next >= MENU_LINES ? next - MENU_LINES : 0),
                "L did not move to the previous menu page");
        select_row(original_row);
    }
    expect_screen(original_screen);
}

static void open_room_actions(void) {
    expect_screen(EXPLORE);
    for (unsigned i = 0; focus != nearby.count && i <= nearby.count; i++) tap_button('r');
    require(focus == nearby.count, "Room actions could not be focused");
    tap_button('a');
    expect_screen(ROOM_ACTIONS);
}

static void room_option(const char *name) {
    expect_screen(ROOM_ACTIONS);
    unsigned row = 0;
    while (row < list_count() && strcmp(room_options[row].name, name)) row++;
    select_row(row);
    tap_button('a');
}

static unsigned browse_targets(Screen target_screen) {
    expect_screen(target_screen);
    browse_rows();
    unsigned count = list_count();
    for (unsigned i = 0; i < count; i++) {
        unsigned target = target_screen == BAG ? carried.id[i] : nearby.id[i];
        select_row(i);
        tap_button('a');
        expect_screen(VERBS);
        require(action_object == target, "action menu opened for the wrong target");
        browse_rows();
        for (unsigned row = 0; row < action_count; row++) {
            ActionKind kind = action_list[row].kind;
            Screen picker;
            switch (kind) {
                case PICK_TARGET: case PICK_SELF: case PICK_ITEM: picker = SECOND_OBJECT; break;
                case PICK_DIRECTION: picker = HEADING; break;
                case PICK_DIGIT: case PICK_YEAR: picker = DIAL; break;
                case SHOW_CATEGORIES: picker = CATEGORIES; break;
                case SHOW_ORDERS: picker = ORDERS_MENU; break;
                default: continue;
            }
            select_row(row);
            tap_button('a');
            expect_screen(picker);
            if (picker == DIAL) {
                tap_button('u');
                tap_button('d');
            } else browse_rows();
            if (picker == CATEGORIES) {
                unsigned groups_count = list_count();
                for (unsigned group = 0; group < groups_count; group++) {
                    select_row(group);
                    tap_button('a');
                    expect_screen(GROUP);
                    browse_rows();
                    tap_button('b');
                    expect_screen(CATEGORIES);
                    require(selected == group, "Back did not restore the selected category");
                }
            }
            tap_button('b');
            expect_screen(VERBS);
            require(selected == row, "Back did not return to the selected action");
        }
        tap_button('b');
        expect_screen(target_screen);
        require(selected == i, "Back did not restore the selected target");
    }
    tap_button('b');
    return count;
}

static void walk_to(unsigned direction, unsigned destination) {
    tap_button('u');
    expect_screen(PATHS);
    unsigned row = 0;
    while (row < path_count && path_options[row] != direction) row++;
    if (row == path_count) {
        fprintf(stderr, "%s:%u: %s missing from Paths in room %u\n",
                fixture, fixture_line, path_names[direction], room_object);
        exit(EXIT_FAILURE);
    }
    select_row(row);
    tap_button('a');
    require(busy && !strcmp(pending_command, path_commands[direction]), "Paths sent the wrong direction");
    finish_turn();
    require(room_object == destination, "Paths reached the wrong destination");
}

static void browse_room(unsigned *paths, unsigned *targets) {
    expect_screen(EXPLORE);
    unsigned room = z_global(0), score = z_global(1), turn = z_global(2);
    uint32_t random = z_resume().random;
    unsigned original_page = page;
    if (page_count > 1 && !page) {
        tap_button('>');
        require(page == 1, "Right did not advance the story page");
        tap_button('<');
        require(page == original_page, "Left did not restore the story page");
    }
    unsigned original_focus = focus;
    for (unsigned i = 0; i <= nearby.count; i++) {
        tap_button('r');
        require(focus == (original_focus + i + 1) % (nearby.count + 1), "focus skipped a room target");
    }
    tap_button('u');
    expect_screen(PATHS);
    *paths = path_count;
    browse_rows();
    tap_button('b');
    expect_screen(EXPLORE);

    open_room_actions();
    browse_rows();
    room_option("Speak...");
    expect_screen(SPEAK);
    browse_rows();
    select_row(0);
    tap_button('a');
    expect_screen(LETTERS);
    tap_button('>');
    tap_button('a');
    expect_screen(WORDS);
    tap_button('r');
    tap_button('l');
    if (word_count) {
        tap_button('a');
        expect_screen(LETTERS);
        require(spoken_count == 1, "word picker did not append a word");
        tap_button('s');
        require(spoken_count == 0, "word picker did not erase the selected word");
        tap_button('a');
        expect_screen(WORDS);
    }
    tap_button('b');
    expect_screen(LETTERS);
    tap_button('b');
    expect_screen(SPEAK);
    tap_button('b');
    expect_screen(ROOM_ACTIONS);
    room_option("Choose command words...");
    expect_screen(LETTERS);
    require(word_mode == WORDS_COMMAND && !spoken_count, "command word picker did not start empty");
    tap_button('b');
    expect_screen(ROOM_ACTIONS);
    require(room_options[selected].destination == LETTERS, "Back did not restore the command words row");
    room_option("Nearby objects");
    *targets = browse_targets(NEARBY);
    expect_screen(ROOM_ACTIONS);
    tap_button('b');
    expect_screen(EXPLORE);
    tap_button('s');
    *targets += browse_targets(BAG);
    expect_screen(EXPLORE);
    tap_button('t');
    expect_screen(PAUSE);
    browse_rows();
    select_row(1);
    tap_button('a');
    expect_screen(STORY_LOG);
    tap_button('r');
    tap_button('l');
    tap_button('b');
    expect_screen(PAUSE);
    select_row(4);
    tap_button('a');
    expect_screen(MAP);
    tap_button('>');
    tap_button('r');
    tap_button('s');
    expect_screen(PLACES);
    browse_rows();
    tap_button('b');
    expect_screen(MAP);
    tap_button('b');
    expect_screen(PAUSE);
    select_row(5);
    tap_button('a');
    expect_screen(TEXT_REVIEW);
    tap_button('>');
    tap_button('b');
    expect_screen(PAUSE);
    select_row(7);
    tap_button('a');
    expect_screen(CONTROLS);
    tap_button('b');
    expect_screen(PAUSE);
    select_row(PAUSE_HINTS_ROW);
    tap_button('a');
    expect_screen(HINT_BROWSER);
    require(hint_menu == 0 && selected == 0, "Hints did not open at the chapter index");
    render();
    if (current_room_hint()) {
        tap_button('a');
        expect_screen(HINTS);
        require(!hint_topic && !hint_step, "opening local hints revealed a spoiler");
        for (unsigned i = 1; i <= HINT_STEPS; i++) {
            tap_button('a');
            require(hint_step == i, "A did not reveal exactly one hint");
            render();
        }
        tap_button('a');
        require(hint_step == HINT_STEPS, "hint cursor advanced beyond the last hint");
        tap_button('<');
        require(hint_step == HINT_STEPS - 1, "Left did not return to the previous hint");
        tap_button('b');
        expect_screen(HINT_BROWSER);
    }
    for (unsigned row = room_hint_rows(); row < local_hint_rows(); row++) {
        select_row(row);
        const ContextHint *context = hint_browser_context(row);
        require(context && context->room == room && context->chapter == chapter,
                "the hint browser suggested a different room");
        tap_button('a');
        expect_screen(HINTS);
        require(hint_topic == &invisiclues[chapter][context->entry] && hint_context_label == context->label &&
                !hint_step && !hint_offset, "opening a contextual topic revealed or selected the wrong clue");
        tap_button('a');
        require(hint_step == 1 && hint_text() == hint_topic->clues[0],
                "the contextual topic did not reveal its first clue");
        render();
        tap_button('b');
        expect_screen(HINT_BROWSER);
        require(selected == row, "Back did not return to the contextual topic");
    }
    select_row(local_hint_rows());
    tap_button('a');
    expect_screen(NEXT_STEPS);
    render();
    tap_button('b');
    expect_screen(HINT_BROWSER);
    require(is_next_steps_row(selected), "Back did not return to Next steps");
    tap_button('b');
    expect_screen(PAUSE);
    require(selected == PAUSE_HINTS_ROW, "Back did not return to Hints");
    tap_button('a');
    expect_screen(HINT_BROWSER);
    tap_button('b');
    expect_screen(PAUSE);
    tap_button('t');
    expect_screen(EXPLORE);
    require(!busy && z_global(0) == room && z_global(1) == score && z_global(2) == turn &&
            z_resume().random == random && page == original_page,
            "browsing changed room, score, turns, random state, or story page");
}

static void look_around(void) {
    open_room_actions();
    room_option("Look");
    require(busy && !strcmp(pending_command, "look"), "Look did not submit the expected command");
    finish_turn();
}

static void open_target(unsigned target) {
    open_room_actions();
    room_option("Nearby objects");
    expect_screen(NEARBY);
    unsigned row = 0;
    while (row < nearby.count && nearby.id[row] != target) row++;
    select_row(row);
    tap_button('a');
    expect_screen(VERBS);
    require(action_object == target, "opened the wrong target menu");
}

static bool examine_target(unsigned target, const char *screenshot, FILE *report) {
    open_target(target);
    char name[Z_NAME_MAX];
    story_noun(target, name);
    for (unsigned row = 0; row < action_count; row++) {
        Action action = action_list[row];
        if (action.kind != DO_OBJECT || strcmp(action.verb, "examine")) continue;
        select_row(row);
        tap_button('a');
        require(busy && !strncmp(pending_command, "examine ", 8), "Examine submitted the wrong command");
        char command[COMMAND_SIZE];
        strcpy(command, pending_command);
        finish_turn();
        fprintf(report, "%s\t%u\t%s\t%s\t%u\t%s\n", screenshot, target, name,
                command, room_object, response_size ? "text" : "empty");
        return true;
    }
    fprintf(report, "%s\t%u\t%s\t\t\tunavailable\n", screenshot, target, name);
    return false;
}

static unsigned exploration_row(unsigned target, const char *name) {
    // Prefer an object's specific interaction before the generic Take action.
    static const struct { const char *verb; bool control_only; } choices[] = {
        {"read plaque", false}, {"read", false}, {"turn on", false}, {"turn off", false},
        {"open", false}, {"close", false}, {"press", true}, {"push", true}, {"pull", true},
        {"take", false}
    };
    bool control = text_has(name, "button") || text_has(name, "lever") ||
                   text_has(name, "switch") || text_has(name, "panel");
    for (unsigned i = 0; i < sizeof(choices) / sizeof(choices[0]); i++) {
        const char *verb = choices[i].verb;
        if (choices[i].control_only && !control) continue;
        if (!strcmp(verb, "take") && !z_attr(target, ATTR_PORTABLE)) continue;
        for (unsigned row = 0; row < action_count; row++) {
            Action action = action_list[row];
            if ((action.kind == DO_OBJECT || action.kind == DO_LITERAL) && action.verb &&
                !strcmp(action.verb, verb)) return row;
        }
    }
    return action_count;
}

static void write_response(FILE *report) {
    bool separator = false, written = false;
    for (const char *p = response; *p; p++) {
        if (*p == '\n' || *p == '\r' || *p == '\t' || *p == ' ') {
            separator = written;
        } else {
            if (separator) fputc(' ', report);
            fputc(*p, report);
            separator = false;
            written = true;
        }
    }
    fputc('\n', report);
}

static bool explore_target(unsigned target, const char *screenshot, FILE *report) {
    open_target(target);
    char name[Z_NAME_MAX];
    story_noun(target, name);
    unsigned row = exploration_row(target, name);
    if (row == action_count) return false;

    Action action = action_list[row];
    char command[COMMAND_SIZE];
    require(action_command(action, target, NULL, command), "could not form exploratory command");
    unsigned room_before = room_object, score_before = z_global(1), parent_before = z_parent(target);
    bool open_before = z_attr(target, ATTR_OPEN), lit_before = z_attr(target, ATTR_LIT);
    bool dark_before = dark;
    select_row(row);
    tap_button('a');
    require(busy && !strcmp(pending_command, command), "exploratory menu sent the wrong command");
    finish_turn();
    if (action.kind == DO_LITERAL && !strcmp(command, "read plaque"))
        require(text_has(response, "The plaque explains") || text_has(response, "DIMWIT"),
                "Jewel Room plaque text did not appear");

    fprintf(report, "%s\t%u\t%s\t%s\t%u\t%u\t%u\t%u\t%u\t%u\t%d\t%d\t%d\t%d\t%d\t%d\t",
            screenshot, target, name, command, room_before, room_object, score_before, z_global(1),
            parent_before, z_parent(target), open_before, z_attr(target, ATTR_OPEN),
            lit_before, z_attr(target, ATTR_LIT), dark_before, dark);
    write_response(report);
    return true;
}

static void restore_checkpoint(unsigned game, const char *save) {
    static const unsigned seeds[] = {7, 4372, 15};
    require(game < CHAPTERS, "invalid checkpoint chapter");
    start_story(game, seeds[game]);
    FILE *file = fopen(save, "rb");
    require(file != NULL, "could not open checkpoint");
    require(fread(qa_sram, 1, sizeof(qa_sram), file) == sizeof(qa_sram) && fgetc(file) == EOF &&
            !ferror(file), "invalid checkpoint size");
    fclose(file);
    require(save_exists(qa_sram, game), "checkpoint contains no save for this chapter");
    restore_saved_game();
    require(busy, "checkpoint did not restore");
    finish_turn();
}

static void choose_action(const char *verb, const char *argument) {
    require(screen == VERBS || screen == GROUP, "expected an object action menu");
    unsigned row = 0;
    for (; row < action_count; row++) {
        Action action = action_list[row];
        if (action.verb && !strcmp(action.verb, verb) &&
            (!argument || (action.argument && !strcmp(action.argument, argument)))) break;
    }
    if (row == action_count) {
        fprintf(stderr, "%s:%u: action '%s' is unavailable for target %u\n",
                fixture, fixture_line, verb, action_object);
        exit(EXIT_FAILURE);
    }
    select_row(row);
    tap_button('a');
}

static void choose_verb(const char *verb) {
    choose_action(verb, NULL);
}

static void choose_group(ActionGroup group) {
    expect_screen(VERBS);
    unsigned row = 0;
    while (row < action_count && action_list[row].kind != SHOW_CATEGORIES) row++;
    select_row(row);
    tap_button('a');
    expect_screen(CATEGORIES);
    for (row = 0; row < list_count() && groups[row] != group; row++) {}
    select_row(row);
    tap_button('a');
    expect_screen(GROUP);
}

static void open_bag_item(const char *name) {
    expect_screen(EXPLORE);
    tap_button('s');
    expect_screen(BAG);
    unsigned row = 0;
    for (; row < carried.count; row++) {
        char noun[Z_NAME_MAX];
        story_noun(carried.id[row], noun);
        if (!strcmp(noun, name)) break;
    }
    if (row == carried.count) {
        fprintf(stderr, "%s:%u: '%s' is missing from the bag\n", fixture, fixture_line, name);
        exit(EXIT_FAILURE);
    }
    select_row(row);
    tap_button('a');
    expect_screen(VERBS);
}

static void finish_command(const char *expected) {
    require(busy && !strcmp(pending_command, expected), "menu sent the wrong command");
    finish_turn();
}

static void choose_argument(const char *name) {
    expect_screen(SECOND_OBJECT);
    unsigned row = 0;
    for (; row < second.count; row++) {
        char noun[Z_NAME_MAX];
        story_noun(second.id[row], noun);
        if (!strcmp(noun, name)) break;
    }
    select_row(row);
    tap_button('a');
}

static void submit_spoken_word(unsigned speech_row, const char *word, const char *expected) {
    open_room_actions();
    room_option("Speak...");
    expect_screen(SPEAK);
    select_row(speech_row);
    tap_button('a');
    expect_screen(LETTERS);
    for (unsigned i = 0; i < (unsigned)(word[0] - 'a'); i++) tap_button('>');
    tap_button('a');
    expect_screen(WORDS);
    unsigned row = 0;
    for (; row < word_count; row++) {
        char candidate[Z_WORD_MAX];
        speech_word(words[row], candidate);
        if (!strcmp(candidate, word)) break;
    }
    require(row < word_count, "word is missing from the word picker");
    for (unsigned i = 0; i < row; i++) tap_button('d');
    tap_button('a');
    expect_screen(LETTERS);
    tap_button('t');
    finish_command(expected);
}

static void check_clue_word(const char *word, bool visible) {
    open_room_actions();
    room_option("Choose command words...");
    expect_screen(LETTERS);
    for (unsigned i = 0; i < (unsigned)(word[0] - 'a'); i++) tap_button('>');
    tap_button('a');
    expect_screen(WORDS);
    bool found = false;
    for (unsigned row = 0; row < word_count; row++) {
        char candidate[Z_WORD_MAX];
        speech_word(words[row], candidate);
        if (!strcmp(candidate, word)) found = true;
    }
    require(found == visible, "Cyclops name visibility did not match the clue state");
    tap_button('b');
    expect_screen(LETTERS);
    tap_button('b');
    expect_screen(ROOM_ACTIONS);
    tap_button('b');
    expect_screen(EXPLORE);
}

static void read_cyclops_clue(void) {
    open_target(196);
    choose_verb("read");
    finish_command("read black book");
    require(text_has(response, "Commandment #12592"), "the book did not show the commandment");
}

static void check_cyclops_clue_visibility(void) {
    fixture = "Zork I Cyclops clue visibility";
    fixture_line = 0;
    restore_checkpoint(0, "qa/mgba-room-gallery/route-data/zork-i-mine-audit/route-129-lit.sav");
    check_clue_word("odysseus", false);
    check_clue_word("ulysses", false);
    read_cyclops_clue();
    check_clue_word("odysseus", true);
    check_clue_word("ulysses", true);

    restore_saved_game();
    require(busy, "could not restore the save from before reading");
    finish_turn();
    check_clue_word("odysseus", false);
    check_clue_word("ulysses", false);
    read_cyclops_clue();
    tap_button('t');
    expect_screen(PAUSE);
    select_row(2);
    tap_button('a');
    require(text_has(response, "Game saved."), "could not save after reading the clue");
    visual_state_reset();
    restore_saved_game();
    require(busy, "could not restore the save from after reading");
    finish_turn();
    check_clue_word("odysseus", true);
    check_clue_word("ulysses", true);
    puts("Zork I button replay passed Cyclops clue discovery and save/restore.");
}

static void check_learned_spells(void) {
    fixture = "Zork II learned spells";
    fixture_line = 0;
    restore_checkpoint(1, "qa/mgba-room-gallery/route-data/zork-ii-finish/re-audit/route-16-lit.sav");
    require(!visual_state_wizard_spells(), "old checkpoint unexpectedly knows a spell");
    open_room_actions();
    room_option("Speak...");
    expect_screen(SPEAK);
    unsigned ordinary_options = sizeof(speech_options) / sizeof(speech_options[0]);
    require(list_count() == ordinary_options, "spell shortcuts appeared before hearing a spell");
    tap_button('b');
    tap_button('b');

    for (unsigned i = 0; i < 60 && !visual_state_wizard_spells(); i++) {
        open_room_actions();
        room_option("Wait");
        finish_command("wait");
    }
    uint16_t heard = visual_state_wizard_spells();
    const char *spell = learned_spell_at(0);
    require(spell && !learned_spell_at(1) && text_has(response, spell),
            "the Wizard's dialogue did not reveal exactly the audible spell");

    tap_button('t');
    expect_screen(PAUSE);
    select_row(2);
    tap_button('a');
    require(text_has(response, "Game saved."), "could not save the learned spell");
    visual_state_reset();
    restore_saved_game();
    require(busy, "could not restore the learned spell");
    finish_turn();
    require(visual_state_wizard_spells() == heard, "Restore did not recover the exact spell list");

    open_room_actions();
    room_option("Speak...");
    expect_screen(SPEAK);
    require(list_count() == ordinary_options + 1, "Speak did not add the learned spell menu");
    select_row(ordinary_options);
    tap_button('a');
    expect_screen(LEARNED_SPELLS);
    require(list_count() == 1 && learned_spell_at(0) == spell, "learned spell list is incorrect");
    tap_button('b');
    expect_screen(SPEAK);
    require(selected == ordinary_options, "Back did not return to Learned spells");
    tap_button('a');
    expect_screen(LEARNED_SPELLS);
    tap_button('a');
    char expected[COMMAND_SIZE];
    require(speech_command(WORDS_SAY, spell, expected), "could not build the spell command");
    finish_command(expected);
    puts("Zork II button replay passed learning, saving, restoring, and selecting a Wizard spell.");
}

static void check_boat_launch(void) {
    fixture = "Zork I boat launch";
    fixture_line = 0;
    restore_checkpoint(0, "qa/mgba-room-gallery/route-data/zork-i-mine-audit/boat-on-land.sav");
    unsigned player = chapters[chapter].player;
    require(room_object == 111 && z_parent(player) != 208, "expected to be ashore at Dam Base");
    open_target(208);
    choose_verb("enter");
    finish_command("enter magic boat");
    require(room_object == 111 && z_parent(player) == 208, "boarding did not put the player in the boat");
    open_room_actions();
    room_option("Launch");
    finish_command("launch");
    require(room_object == 37 && z_parent(player) == 208 && text_has(response, "Frigid River"),
            "launch did not carry the occupied boat onto the river");
    puts("Zork I button replay passed boarding and launching the boat.");
}

static void check_bat_routes(void) {
    fixture = "Zork I vampire bat";
    fixture_line = 0;
    restore_checkpoint(0, "qa/mgba-room-gallery/route-data/zork-i-mine-audit/bat-safe.sav");
    require(room_object == 235, "expected the Bat Room with garlic");
    walk_to(2, 171);
    walk_to(0, 235);
    require(text_has(response, "holding his nose"), "garlic did not deter the bat");
    walk_to(2, 171);
    open_bag_item("clove of garlic");
    choose_verb("drop");
    finish_command("drop clove of garlic");
    walk_to(0, 72);
    require(text_has(response, "swoops down") && text_has(response, "Ladder Bottom"),
            "entering without garlic did not trigger the bat carryaway");
    puts("Zork I button replay passed the Bat Room with and without garlic.");
}

static void check_well_bucket(void) {
    fixture = "Zork II well bucket";
    fixture_line = 0;
    restore_checkpoint(1, "qa/mgba-room-gallery/route-data/zork-ii-art/re-audit/bucket-before-rise.sav");
    unsigned player = chapters[chapter].player;
    require(room_object == 119 && z_parent(player) == 52, "expected to be in the bucket at the well bottom");
    open_bag_item("water");
    choose_group(COMBINE);
    choose_action("pour", "in");
    choose_argument("bucket");
    finish_command("pour water in bucket");
    require(room_object == 8 && z_parent(player) == 52 && text_has(response, "bucket rises"),
            "water did not raise the occupied bucket");
    open_bag_item("teapot");
    choose_verb("fill");
    finish_command("fill teapot");
    require(room_object == 119 && z_parent(player) == 52 && text_has(response, "bucket descends"),
            "recovering the water did not lower the bucket");
    open_target(52);
    choose_verb("stand");
    finish_command("stand");
    require(z_parent(player) != 52, "standing did not let the player out of the bucket");
    puts("Zork II button replay passed the well bucket round trip.");
}

static void check_aquarium_sphere(void) {
    fixture = "Zork II aquarium sphere";
    fixture_line = 0;
    restore_checkpoint(1, "qa/mgba-room-gallery/route-data/zork-ii-art/re-audit/aquarium-intact.sav");
    require(room_object == ZORK2_AQUARIUM_ROOM, "expected the Aquarium Room");
    unsigned score = z_global(1);
    open_bag_item("elvish sword");
    choose_group(COMBINE);
    choose_verb("throw");
    choose_argument("aquarium");
    finish_command("throw elvish sword at aquarium");
    require(text_has(response, "shatters the glass") && z_parent(17) == room_object,
            "throwing the sword did not break the aquarium and expose the dead serpent");
    open_target(31);
    choose_verb("take");
    finish_command("take clear sphere");
    require(z_parent(31) == chapters[chapter].player && z_global(1) == score + 20,
            "the clear sphere did not enter the bag or award its points");
    puts("Zork II button replay passed breaking the aquarium and taking the sphere.");
}

static void check_beam_entry(void) {
    fixture = "Zork III beam and mirror entry";
    fixture_line = 0;
    restore_checkpoint(2, "qa/mgba-room-gallery/route-data/zork-iii-audit/beam-before.sav");
    require(room_object == ZORK3_BEAM_ROOM, "expected the Beam Room");
    open_target(27);
    choose_group(TOOLS);
    choose_verb("break");
    choose_argument("sword");
    finish_command("break red beam of light with sword");
    require(z_parent(202) == room_object && text_has(response, "beam is now interrupted"),
            "the sword did not stay on the floor and interrupt the beam");
    walk_to(2, ZORK3_BUTTON_ROOM);
    open_target(191);
    choose_verb("push");
    finish_command("push red button");
    require(text_has(response, "Click"), "button did not respond to the interrupted beam");
    walk_to(0, ZORK3_BEAM_ROOM);
    walk_to(0, 169);
    walk_to(0, 160);
    require(text_has(response, "Inside Mirror"), "the button did not open the route into the mirror");
    puts("Zork III button replay passed beam interruption and mirror entry.");
}

static void check_mirror_rotation(void) {
    fixture = "Zork III mirror rotation";
    fixture_line = 0;
    restore_checkpoint(2, "qa/mgba-room-gallery/route-data/zork-iii-audit/mirror-initial.sav");
    require(room_object == 160, "expected to be inside the mirror");
    open_target(197);
    choose_verb("push");
    finish_command("push red panel");
    require(text_has(response, "doesn't move"), "lowered pole did not block rotation");
    open_target(2);
    choose_verb("lift");
    finish_command("lift short pole");
    require(text_has(response, "above the floor"), "short pole did not lift");
    open_target(212);
    choose_verb("push");
    finish_command("push yellow panel");
    require(room_object == 160 && text_has(response, "rotates clockwise") &&
            text_has(response, "indicates northwest"), "mirror did not rotate to northwest");
    puts("Zork III button replay passed blocked rotation and rotation after lifting the pole.");
}

static void check_grating_route(void) {
    fixture = "Zork I locked grating";
    fixture_line = 0;
    restore_checkpoint(0, "qa/mgba-room-gallery/route-data/zork-i-mine-audit/grating-locked-below.sav");
    require(room_object == 216, "expected the locked Grating Room");
    walk_to(8, 216);
    require(text_has(response, "grating is closed"), "locked grating did not block the way up");

    open_target(29);
    choose_verb("unlock");
    choose_argument("skeleton key");
    require(busy && !strcmp(pending_command, "unlock grating with skeleton key"),
            "Unlock sent the wrong command");
    finish_turn();
    require(text_has(response, "grate is unlocked"), "skeleton key did not unlock the grating");
    open_target(29);
    choose_verb("open");
    require(busy && !strcmp(pending_command, "open grating"), "Open sent the wrong command");
    finish_turn();
    require(z_attr(29, ATTR_OPEN), "grating did not open");
    walk_to(8, 160);
    walk_to(9, 216);
    puts("Zork I button replay passed the locked grating and both crossings.");
}

static void check_safe_blast(void) {
    fixture = "Zork II safe blast";
    fixture_line = 0;
    restore_checkpoint(1, "qa/mgba-room-gallery/route-data/zork-ii-art/re-audit/fuse-burning.sav");
    require(room_object == 182, "expected the Dusty Room with the fuse burning");
    unsigned score = z_global(1), turns = z_global(2);
    open_room_actions();
    room_option("Wait");
    require(busy && !strcmp(pending_command, "wait"), "Wait sent the wrong command");
    finish_turn();
    require(room_object == 224 && text_has(response, "You have died"),
            "waiting by the fuse did not cause the expected death");

    tap_button('t');
    expect_screen(PAUSE);
    select_row(3);
    tap_button('a');
    expect_screen(RESTORE_CONFIRM);
    select_row(1);
    tap_button('a');
    require(busy, "Restore did not load the pre-blast save");
    finish_turn();
    require(room_object == 182 && z_global(1) == score && z_global(2) == turns,
            "Restore did not recover the pre-blast room, score, and turn");
    walk_to(0, 143);
    require(text_has(response, "explosion nearby"), "escaping did not trigger the explosion from safety");
    walk_to(2, 182);
    require(z_global(1) == score && text_has(response, "door has been blown off"),
            "returning did not reveal the blasted safe at the original score");
    puts("Zork II button replay passed the fatal blast, Restore, and safe escape.");
}

static void check_guardian_revival(void) {
    fixture = "Zork III unprotected Guardians";
    fixture_line = 0;
    restore_checkpoint(2, "qa/mgba-room-gallery/route-data/zork-iii-audit/guardians-before-potion.sav");
    require(room_object == 54, "expected the approach to the Guardians");
    walk_to(0, 211);
    require(text_has(response, "You have died") && text_has(response, "awakening"),
            "Guardians did not trigger the expected death and revival");
    expect_screen(EXPLORE);
    look_around();
    require(room_object == 211 && text_has(response, "Endless Stair"),
            "player could not resume exploration at the Endless Stair");
    puts("Zork III button replay passed the Guardian death and revival.");
}

static void check_cyclops_offerings(void) {
    fixture = "Zork I Cyclops offerings";
    fixture_line = 0;
    restore_checkpoint(0, "qa/mgba-room-gallery/route-data/zork-i-mine-audit/cyclops-food-start.sav");
    require(room_object == 78, "expected the Cyclops Room");
    walk_to(8, 78);
    require(text_has(response, "let you past"), "awake Cyclops did not block the stairs");

    open_target(199);
    choose_verb("give");
    choose_argument("lunch");
    require(busy && !strcmp(pending_command, "give lunch to cyclops"), "food offering sent the wrong command");
    finish_turn();
    require(room_object == 78 && text_has(response, "hot peppers"), "Cyclops did not accept the food");

    open_target(199);
    choose_verb("give");
    choose_argument("glass bottle");
    require(busy && text_has(pending_command, "bottle to cyclops"), "drink offering sent the wrong command");
    finish_turn();
    require(room_object == 78 && text_has(response, "falls fast asleep"), "Cyclops did not fall asleep");
    puts("Zork I button replay passed the blocked stair and Cyclops food-and-drink solution.");
}

static void answer_riddle(const char *word) {
    char expected[COMMAND_SIZE];
    snprintf(expected, sizeof(expected), "answer \"%s\"", word);
    submit_spoken_word(1, word, expected);
}

static void check_riddle_retry(void) {
    fixture = "Zork II riddle retry";
    fixture_line = 0;
    restore_checkpoint(1, "qa/mgba-room-gallery/route-data/zork-ii-art/re-audit/riddle-closed.sav");
    require(room_object == 238, "expected the closed Riddle Room");
    unsigned score = z_global(1);
    walk_to(1, 238);
    require(text_has(response, "stone door is closed"), "closed riddle door did not block east");
    answer_riddle("water");
    require(z_global(1) == score && text_has(response, "hollow laugh"), "wrong answer was not rejected");
    check_replay_hint(&replay_hint_checkpoints[1], true);
    walk_to(1, 238);
    require(text_has(response, "stone door is closed"), "wrong answer opened the door");
    answer_riddle("well");
    require(z_global(1) == score + 5, "correct riddle answer did not award five points");
    check_replay_hint(&replay_hint_checkpoints[1], false);
    walk_to(1, 38);
    walk_to(3, 238);
    require(z_global(1) == score + 5 && z_attr(7, ATTR_OPEN), "door did not remain open on return");
    puts("Zork II button replay passed the wrong riddle answer, retry, and round trip.");
}

static void check_puzzle_wall(void) {
    fixture = "Zork III Royal Puzzle wall";
    fixture_line = 0;
    restore_checkpoint(2, "qa/mgba-room-gallery/route-data/zork-iii-audit/puzzle-initial.sav");
    require(room_object == ROYAL_PUZZLE_ROOM, "expected the Royal Puzzle");
    look_around();
    require(puzzle_map.valid, "Look did not survey the puzzle");
    PuzzleMap original_map = puzzle_map;
    unsigned score = z_global(1);
    tap_button('t');
    expect_screen(PAUSE);
    select_row(2);
    tap_button('a');
    expect_screen(EXPLORE);
    require(text_has(response, "Game saved."), "could not save the surveyed puzzle");

    open_target(TARGET_EAST_WALL);
    choose_verb("push");
    require(busy && !strcmp(pending_command, "push east wall"), "wall picker sent the wrong command");
    finish_turn();
    require(text_has(response, "wall slides forward") && z_global(1) == score + 1,
            "pushing the wall did not advance the puzzle");
    walk_to(2, ROYAL_PUZZLE_ROOM);
    require(puzzle_map.valid && memcmp(original_map.tiles, puzzle_map.tiles, sizeof(puzzle_map.tiles)),
            "wall push and movement did not change the surveyed map");

    tap_button('t');
    expect_screen(PAUSE);
    select_row(3);
    tap_button('a');
    expect_screen(RESTORE_CONFIRM);
    select_row(1);
    tap_button('a');
    require(busy, "Restore did not load the surveyed puzzle");
    finish_turn();
    require(room_object == ROYAL_PUZZLE_ROOM && z_global(1) == score && puzzle_map.valid &&
            !memcmp(original_map.tiles, puzzle_map.tiles, sizeof(puzzle_map.tiles)),
            "Restore did not recover the original Royal Puzzle map and score");
    puts("Zork III button replay passed the Royal Puzzle wall push, movement, and map Restore.");
}

static void browse_zork3_ending(void) {
    run_route(2, "tests/fixtures/zork3-button-route.tsv", 15, ZORK3_TREASURY, 7, NULL, stderr);
    fixture = "Zork III ending";
    fixture_line = 0;
    expect_screen(FINISHED);
    require(page_count > 1 && page == 0, "ending did not start at the first story page");
    tap_button('r');
    require(page == 1, "R did not advance the ending text");
    tap_button('l');
    require(page == 0, "L did not return to the first ending page");
    for (unsigned i = 0; i < page_count && screen == FINISHED; i++) tap_button('a');
    expect_screen(FINISHED_MENU);
    tap_button('d');
    require(selected == 1, "ending menu did not reach Restore");
    tap_button('d');
    require(selected == 2, "ending menu did not reach Quit");
    tap_button('b');
    expect_screen(FINISHED);
    require(page == page_count - 1, "Back did not restore the last ending page");
    tap_button('a');
    expect_screen(FINISHED_MENU);
    tap_button('u');
    require(selected == 2, "ending menu did not wrap to Quit");
    tap_button('a');
    expect_screen(LAUNCHER);
    tap_button('d');
    require(chapter == 0, "launcher did not wrap to Zork I");
    tap_button('u');
    require(chapter == 2, "launcher did not return to Zork III");
    tap_button('a');
    expect_screen(SLOT);
    browse_rows();
    tap_button('b');
    expect_screen(LAUNCHER);
}

static void check_puzzle_restore(void) {
    unsigned room = room_object, turns = z_global(2), score = z_global(1);
    look_around();
    expect_screen(EXPLORE);
    tap_button('t');
    expect_screen(PAUSE);
    select_row(3);
    tap_button('a');
    expect_screen(RESTORE_CONFIRM);
    browse_rows();
    tap_button('b');
    expect_screen(PAUSE);
    require(selected == 3, "Back did not return to Restore");
    tap_button('a');
    expect_screen(RESTORE_CONFIRM);
    select_row(1);
    tap_button('a');
    require(busy, "confirming Restore did not load the save");
    finish_turn();
    expect_screen(EXPLORE);
    require(room_object == room && z_global(2) == turns && z_global(1) == score &&
            text_has(response, "Saved game restored."), "Restore did not recover the Royal Puzzle state");
}

static void enter_gallery_room(unsigned game, const char *save, unsigned start_room,
                               unsigned room, const char *step) {
    restore_checkpoint(game, save);
    require(room_object == start_room, "checkpoint restored to the wrong room");
    while (*step == '\t' || *step == ',') {
        if (step[1] == '\n') break;
        unsigned direction, destination;
        int consumed = 0;
        require(sscanf(step + 1, "%u:%u%n", &direction, &destination, &consumed) == 2 &&
                direction < sizeof(path_commands) / sizeof(path_commands[0]), "invalid Paths replay step");
        walk_to(direction, destination);
        step += consumed + 1;
    }
    require(*step == '\n' || (*step == '\t' && step[1] == '\n'), "unexpected gallery route suffix");
    require(room_object == room, "gallery route reached the wrong room");
}

static void run_gallery(const char *plan, const char *examine_path, const char *action_path) {
    browse_zork3_ending();
    fixture = plan;
    fixture_line = 0;
    FILE *file = fopen(plan, "r");
    require(file != NULL, "could not open gallery button plan");
    FILE *examine_report = fopen(examine_path, "w");
    require(examine_report != NULL, "could not open item Examine report");
    fputs("screen\ttarget_id\ttarget\tcommand\tresult_room\tresponse\n", examine_report);
    FILE *action_report = fopen(action_path, "w");
    require(action_report != NULL, "could not open exploratory action report");
    fputs("screen\ttarget_id\ttarget\tcommand\troom_before\troom_after\tscore_before\tscore_after\t"
          "parent_before\tparent_after\topen_before\topen_after\tlit_before\tlit_after\t"
          "dark_before\tdark_after\tresponse\n", action_report);
    unsigned cases = 0, ending_screens = 0, total_examined = 0, total_items = 0;
    unsigned total_explored = 0;
    char line[1024];
    puts("screen\tstatus\tpaths\ttarget_menus\tlook_room\tlook_response\titems\texamined\tobject_actions");
    fflush(stdout);
    while (fgets(line, sizeof(line), file)) {
        unsigned game, start_room, room;
        char view[16], save[256], screenshot[256];
        int end = 0;
        fixture = plan;
        fixture_line++;
        require(sscanf(line, "%u\t%u\t%u\t%15s\t%255s\t%255s%n",
                       &game, &start_room, &room, view, save, screenshot, &end) == 6,
                "invalid gallery button plan row");
        fixture = screenshot;
        const char *step = line + end;
        if (!strcmp(screenshot, "zork-iii/states/ending-story.png") ||
            !strcmp(screenshot, "zork-iii/states/ending-options.png")) {
            printf("%s\tpassed\t\t\t\t\t\t\t\n", screenshot);
            ending_screens++;
            continue;
        }
        enter_gallery_room(game, save, start_room, room, step);
        if (!strcmp(screenshot, "zork-iii/states/puzzle-restored.png")) check_puzzle_restore();
        unsigned paths, targets;
        browse_room(&paths, &targets);
        look_around();
        unsigned look_room = room_object;
        bool look_response = response_size != 0;
        unsigned examined = 0, explored = 0;
        enter_gallery_room(game, save, start_room, room, step);
        Targets room_items = nearby;
        for (unsigned i = 0; i < room_items.count; i++) {
            if (i) enter_gallery_room(game, save, start_room, room, step);
            examined += examine_target(room_items.id[i], screenshot, examine_report);
            enter_gallery_room(game, save, start_room, room, step);
            explored += explore_target(room_items.id[i], screenshot, action_report);
        }
        total_examined += examined;
        total_items += room_items.count;
        total_explored += explored;
        printf("%s\t%s\t%u\t%u\t%u\t%s\t%u\t%u\t%u\n",
               screenshot, examined == room_items.count ? "passed" : "missing-examine",
               paths, targets, look_room, look_response ? "text" : "empty",
               room_items.count, examined, explored);
        fflush(stdout);
        cases++;
    }
    require(!ferror(file) && cases && ending_screens == 2, "incomplete gallery button plan");
    fclose(file);
    require(!ferror(examine_report), "could not write item Examine report");
    require(fclose(examine_report) == 0, "could not close item Examine report");
    require(!ferror(action_report), "could not write exploratory action report");
    require(fclose(action_report) == 0, "could not close exploratory action report");
    fprintf(stderr, "Examined %u/%u Nearby targets.\n", total_examined, total_items);
    fprintf(stderr, "Submitted %u additional object-specific actions.\n", total_explored);
    fprintf(stderr, "Explored %u gallery states with button controls, including %u ending screens.\n",
            cases + ending_screens, ending_screens);
    fixture = examine_path;
    fixture_line = 0;
    require(total_examined == total_items, "one or more Nearby targets have no selectable Examine action");
}

static void check_hint_section(unsigned *topics, unsigned *clues, unsigned *long_clues) {
    expect_screen(HINT_BROWSER);
    unsigned first = hint_guide_offset(), rows = hint_browser_count();
    for (unsigned row = first; row < rows; row++) {
        select_row(row);
        const HintEntry *entry = hint_browser_entry(selected);
        tap_button('a');
        if (!entry->clues) {
            check_hint_section(topics, clues, long_clues);
        } else {
            expect_screen(HINTS);
            require(!hint_step && !hint_offset, "opening a guide topic revealed a clue");
            (*topics)++;
            for (unsigned step = 1; step <= entry->count; step++) {
                tap_button('a');
                require(hint_step == step && !hint_offset && hint_text() == entry->clues[step - 1],
                        "guide did not reveal the correct clue");
                require(!text_has(hint_text(), "&gt;") && !text_has(hint_text(), "&lt;") &&
                        !text_has(hint_text(), "&amp;") && !text_has(hint_text(), "<pre>") &&
                        !text_has(hint_text(), "<li>"), "guide exposed source markup");
                (*clues)++;
                if (next_hint_page(hint_text(), 0)) {
                    (*long_clues)++;
                    while (next_hint_page(hint_text(), hint_offset) != hint_offset) {
                        unsigned previous = hint_offset;
                        tap_button('d');
                        require(hint_offset > previous, "long clue could not advance to its next page");
                    }
                    while (hint_offset) {
                        unsigned previous = hint_offset;
                        tap_button('u');
                        require(hint_offset < previous, "long clue could not return to its previous page");
                    }
                }
                render();
            }
            tap_button('a');
            require(hint_step == entry->count, "guide advanced past its last clue");
            if (entry->count > 1) {
                tap_button('<');
                require(hint_step == entry->count - 1, "guide could not return to its previous clue");
            }
        }
        tap_button('b');
        expect_screen(HINT_BROWSER);
        require(selected == row, "Back did not return to the guide's selected row");
    }
}

static void check_full_hint_guides(void) {
    static const unsigned expected_topics[] = {90, 94, 88};
    static const unsigned expected_clues[] = {224, 240, 226};
    fixture = "Trilogy InvisiClues guide";
    fixture_line = 0;
    for (unsigned game = 0; game < CHAPTERS; game++) {
        start_story(game, 7);
        unsigned room = room_object, turn = z_global(2), score = z_global(1);
        uint32_t random = z_resume().random;
        tap_button('t');
        expect_screen(PAUSE);
        select_row(PAUSE_HINTS_ROW);
        tap_button('a');
        unsigned topics = 0, clues = 0, long_clues = 0;
        check_hint_section(&topics, &clues, &long_clues);
        require(topics == expected_topics[game] && clues == expected_clues[game] && long_clues,
                "guide topics, clues, or pagination coverage is incomplete");
        tap_button('b');
        expect_screen(PAUSE);
        tap_button('t');
        expect_screen(EXPLORE);
        require(room_object == room && z_global(2) == turn && z_global(1) == score &&
                z_resume().random == random, "browsing guide changed the story");
        printf("Zork %u InvisiClues passed (%u entries, %u clues and appendix pages).\n", game + 1, topics, clues);
    }
}

static void check_context_ship_visibility(void) {
    static const char *const saves[] = {
        "qa/mgba-room-gallery/route-data/zork-iii-audit/route-181-lit.sav",
        "qa/mgba-room-gallery/route-data/zork-iii-audit/sailor-visible.sav"
    };
    fixture = "Zork III contextual ship hint";
    fixture_line = 0;
    for (unsigned i = 0; i < sizeof(saves) / sizeof(saves[0]); i++) {
        restore_checkpoint(2, saves[i]);
        require(room_object == 181, "ship checkpoint did not reach Flathead Ocean");
        tap_button('t');
        expect_screen(PAUSE);
        select_row(PAUSE_HINTS_ROW);
        tap_button('a');
        expect_screen(HINT_BROWSER);
        const ContextHint *hint = context_hint_at(0);
        if (i == 0) require(!hint, "hints revealed the ship before it appeared");
        else require(hint && hint->target == ZORK3_VIKING_SHIP, "visible ship did not get a contextual hint");
    }
    puts("Zork III contextual ship hint passed visible and absent states.");
}

static void open_hint_browser(void) {
    tap_button('t');
    expect_screen(PAUSE);
    select_row(PAUSE_HINTS_ROW);
    tap_button('a');
    expect_screen(HINT_BROWSER);
}

static void close_hint_browser(void) {
    expect_screen(HINT_BROWSER);
    tap_button('b');
    expect_screen(PAUSE);
    tap_button('t');
    expect_screen(EXPLORE);
}

static void open_next_steps(void) {
    expect_screen(HINT_BROWSER);
    select_row(local_hint_rows());
    tap_button('a');
    expect_screen(NEXT_STEPS);
}

static bool shows_next_step(const char *expected) {
    expect_screen(NEXT_STEPS);
    const char *steps[NEXT_STEP_LIMIT];
    unsigned count = story_next_steps(steps);
    for (unsigned i = 0; i < count; i++)
        if (!strcmp(steps[i], expected)) return true;
    return false;
}

static void check_replay_hint(const ReplayHintCheckpoint *checkpoint, bool before) {
    unsigned room = room_object, turn = z_global(2), score = z_global(1);
    unsigned story_page = page, previous_focus = focus, log_size = transcript_used;
    uint32_t random = z_resume().random;
    open_hint_browser();
    if (before) {
        unsigned row = 0;
        if (checkpoint->context) {
            for (row = room_hint_rows(); row < local_hint_rows(); row++) {
                const ContextHint *context = hint_browser_context(row);
                if (context && !strcmp(context->label, checkpoint->context)) break;
            }
            require(row < local_hint_rows(), "the active puzzle did not get its contextual hint");
        } else require(room_hint_rows(), "the current room did not get its local hint");
        select_row(row);
        const HintEntry *topic = hint_browser_entry(row);
        const char *first_clue = topic ? topic->clues[0] : current_room_hint()->steps[0];
        tap_button('a');
        expect_screen(HINTS);
        require(hint_topic == topic && !hint_step && !hint_offset, "opening the topic did not preserve the selected clue");
        tap_button('a');
        require(hint_step == 1 && hint_text() == first_clue, "the puzzle did not reveal its first clue");
        render();
        tap_button('b');
        expect_screen(HINT_BROWSER);
    }
    open_next_steps();
    require(shows_next_step(checkpoint->current_step) == before,
            "the next step did not reflect the puzzle milestone");
    if (checkpoint->next_step)
        require(shows_next_step(checkpoint->next_step) != before,
                "the follow-up suggestion appeared at the wrong point");
    render();
    tap_button('b');
    close_hint_browser();
    require(!busy && room_object == room && z_global(2) == turn && z_global(1) == score &&
            page == story_page && focus == previous_focus && transcript_used == log_size &&
            z_resume().random == random, "examining a hint changed the replay state");
}

static void check_next_steps(const char *expected, bool visible) {
    unsigned room = room_object, turn = z_global(2), score = z_global(1);
    open_hint_browser();
    open_next_steps();
    require(shows_next_step(expected) == visible, "next step did not match exploration and puzzle progress");
    render();
    tap_button('b');
    close_hint_browser();
    require(room_object == room && z_global(2) == turn && z_global(1) == score,
            "reading next steps changed the story");
}

static void check_cross_room_next_steps(void) {
    fixture = "Zork III next steps across rooms";
    fixture_line = 0;
    const char *beam_step = "Bring the sword to the Beam Room.";
    const char *button_step = "Check the controls in the Button Room.";
    start_story(2, 15);
    check_next_steps(beam_step, false);

    restore_checkpoint(2, "qa/mgba-room-gallery/route-data/zork-iii-audit/beam-before.sav");
    walk_to(2, ZORK3_BUTTON_ROOM);
    check_next_steps(beam_step, true);
    check_next_steps(button_step, false);
    walk_to(0, ZORK3_BEAM_ROOM);
    open_target(27);
    choose_group(TOOLS);
    choose_verb("break");
    choose_argument("sword");
    finish_command("break red beam of light with sword");
    check_next_steps(beam_step, false);
    check_next_steps(button_step, true);
    puts("Zork III next steps passed discovery, cross-room suggestions, and puzzle progress.");
}

int main(int argc, char **argv) {
    if (argc == 4) {
        run_gallery(argv[1], argv[2], argv[3]);
        return 0;
    }
    require(argc == 1, "expected optional gallery plan, Examine report, and action report paths");
    run_route(0, "tests/fixtures/zork1-button-route.tsv", 7, 244, 350, NULL, stdout);
    run_route(1, "tests/fixtures/zork2-button-route.tsv", 4372, 230, 400, NULL, stdout);
    run_route(2, "tests/fixtures/zork3-button-route.tsv", 15, ZORK3_TREASURY, 7, NULL, stdout);
    run_route(0, "tests/fixtures/zork1-alternate-button-route.tsv", 7, 244, 350,
              "unlock grating with skeleton key", stdout);
    run_route(1, "tests/fixtures/zork2-alternate-button-route.tsv", 4372, 230, 400,
              "kick dragon", stdout);
    run_route(2, "tests/fixtures/zork3-alternate-button-route.tsv", 15, ZORK3_TREASURY, 7,
              "drink from vial", stdout);
    check_grating_route();
    check_safe_blast();
    check_guardian_revival();
    check_cyclops_offerings();
    check_cyclops_clue_visibility();
    check_learned_spells();
    check_riddle_retry();
    check_puzzle_wall();
    check_boat_launch();
    check_bat_routes();
    check_well_bucket();
    check_aquarium_sphere();
    check_beam_entry();
    check_mirror_rotation();
    check_full_hint_guides();
    check_context_ship_visibility();
    check_cross_room_next_steps();
    return 0;
}
