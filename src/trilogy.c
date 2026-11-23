#include "interaction.h"
#include "room_map.h"
#include "room_visuals.h"
#include "save.h"
#include "transcript_codec.h"
#include "visual_state.h"

enum {
    ART_Y = 15, ART_HEIGHT = 76, TEXT_Y = 91, FOOT_Y = 145,
    STORY_LINES = 4, MENU_LINES = 4, LOG_SCREEN_LINES = 11,
    TRANSCRIPT_CAPACITY = 24576, TRANSCRIPT_ENTRY_CAPACITY = 8192,
    TRANSCRIPT_ENTRIES = 1024
};
#define BLACK RGB(12, 17, 17)
#define INK RGB(18, 25, 23)
#define PANEL RGB(34, 45, 39)
#define LINE RGB(96, 104, 77)
#define GOLD RGB(249, 195, 109)
#define PAPER RGB(250, 233, 201)
#define DIM RGB(170, 184, 160)
#define BACK RGB(115, 208, 196)
#define SRAM ((volatile uint8_t *)0x0E000000)

static const char *find_substring(const char *text, const char *part) {
    if (!*part) return text;
    for (; *text; text++) {
        const char *candidate = text;
        const char *needle = part;
        while (*candidate && *needle && *candidate == *needle) {
            candidate++;
            needle++;
        }
        if (!*needle) return text;
    }
    return 0;
}

typedef enum {
    LAUNCHER, SLOT, EXPLORE, BAG, NEARBY, VERBS, CATEGORIES, GROUP, ORDERS_MENU,
    SECOND_OBJECT, HEADING, DIAL, ROOM_ACTIONS, SPEAK, LETTERS, WORDS,
    PATHS, PAUSE, MAP, PLACES, TEXT_REVIEW, STORY_LOG, COMMAND, CONTROLS, RETURN_CONFIRM, FINISHED,
    FINISHED_MENU
} Screen;

typedef struct { Screen screen; unsigned row; } MenuCursor;

static Screen screen, command_back;
static MenuCursor object_origin, room_menu_origin, category_origin, argument_origin, speech_origin;
static unsigned chapter, focus, selected, room_object;
static uint8_t path_options[12];
static unsigned path_count;
static bool verb_is_bag, busy, has_progress, dark;
static bool restored_snapshot;
static bool finished_prompt, successful_ending;
static unsigned entropy_state;
static uint8_t visited[VISITED_BYTES];
static Trail trail;
static unsigned travel_room, travel_direction, map_room, map_cursor;
static bool map_vertical;
static Targets nearby, carried, second;
static Action action_list[MAX_ACTIONS], pending_action;
static unsigned action_count, action_object, place_count;
static uint8_t places[256];
static char room_name[Z_NAME_MAX], command[COMMAND_SIZE];
static unsigned key_cursor, dial_digits, dial_cursor, category_row;
static uint8_t digits[3];
static unsigned speech_mode, letter_cursor, word_cursor, word_count, spoken_count;
static uint16_t words[1024], spoken[WORDS_PER_PHRASE];
static void collect_paths(void);

static char response[7168];
static uint16_t page_offsets[256];
static unsigned response_size, page_count, page;
static bool text_truncated;
static char transcript[TRANSCRIPT_CAPACITY];
static char transcript_expanded[TRANSCRIPT_ENTRY_CAPACITY];
static uint16_t transcript_start[TRANSCRIPT_ENTRIES], transcript_length[TRANSCRIPT_ENTRIES];
// Wrapped row count per entry, filled lazily; 0 means not yet counted (every entry has a row).
static uint16_t transcript_rows[TRANSCRIPT_ENTRIES];
static unsigned transcript_used, transcript_count, transcript_page;
static char pending_command[COMMAND_SIZE];
static bool transcript_trimmed;
static PuzzleMap puzzle_map;
static unsigned scene_animation_stage(void);

static const struct { const char *name; Screen destination; const char *command; } room_options[] = {
    {"Paths", PATHS, NULL}, {"Speak...", SPEAK, NULL}, {"Nearby objects", NEARBY, NULL},
    {"Listen", EXPLORE, "listen"}, {"Wait", EXPLORE, "wait"}, {"Look", EXPLORE, "look"},
    {"Score", EXPLORE, "score"}, {"Take all", EXPLORE, "take all"}, {"Drop all", EXPLORE, "drop all"},
    {"Pray", EXPLORE, "pray"}, {"Launch", EXPLORE, "launch"}, {"Dive", EXPLORE, "dive"}
};
static const char *const speech_options[] = {"Say...", "Answer...", "Call out...", "Greet...", "Yes", "No"};
static const char *const speech_titles[] = {"Say", "Answer", "Call out", "Greet"};
static const char *const category_options[] = {"Inspect", "Handle", "Use", "Combine", "Use a tool", "Movement"};
static const ActionGroup groups[] = {INSPECT, HANDLE, USE, COMBINE, TOOLS, MOVE};
static const char *const pause_options[] = {
    "Resume", "Story so far", "Save game", "Restore game", "Map", "Read last response",
    "Enter command...", "Controls", "Return to trilogy"
};
static const char *const slot_options[] = {"Continue saved game", "New game", "Back"};
static const char *const return_options[] = {"Keep exploring", "Return to trilogy"};
static const char *const finished_options[] = {"Restart chapter", "Restore saved game", "Quit to trilogy menu"};
static const char keys[] = "abcdefghijklmnopqrstuvwxyz0123456789.,'\"-!?:/()&";

static void number(char *out, int value) {
    char reverse[6];
    unsigned count = 0, n = value < 0 ? -value : value;
    if (value < 0) *out++ = '-';
    do {
        reverse[count++] = '0' + n % 10;
        n /= 10;
    } while (n);
    while (count) *out++ = reverse[--count];
    *out = 0;
}

static int direction_label_width(unsigned direction, const char *const directions[12]) {
    if (direction >= 4 && direction <= 7) return label_width(directions[0]);
    return label_width(directions[direction]);
}

static void draw_direction_label(int x, int y, unsigned direction, const char *const directions[12], Color color) {
    static const uint8_t diagonal_triangles[4][5] = {
        {16, 24, 28, 30, 31},       // Northeast
        {31, 30, 28, 24, 16},       // Southeast
        {31, 15, 7, 3, 1},          // Southwest
        {1, 3, 7, 15, 31},           // Northwest
    };
    if (direction < 4 || direction > 7) {
        label(x, y, directions[direction], color);
        return;
    }

    const uint8_t *rows = diagonal_triangles[direction - 4];
    for (int row = 0; row < 5; row++)
        for (int col = 0; col < 5; col++)
            if (rows[row] & (1 << col)) pixel(x + col, y + row + 1, color);
}

static unsigned direction_to_previous_room(void) {
    if (!trail.count) return TRAIL_DIRECTIONS;
    const TrailLink *last = &trail.links[trail.count - 1];
    if (last->to != room_object) return TRAIL_DIRECTIONS;

    for (unsigned i = 0; i < path_count; i++) {
        unsigned direction = path_options[i];
        const uint8_t *exit;
        size_t size = z_property(room_object, path_properties[direction], &exit);
        if (size == 1 && exit[0] == last->from) return direction;
        if (trail_destination(&trail, room_object, direction) == last->from) return direction;
    }
    return TRAIL_DIRECTIONS;
}

static void draw_header(const char *name, const char *right, bool show_exits) {
    fill(0, 0, SCREEN_W, ART_Y, INK);
    fill(0, ART_Y - 1, SCREEN_W, 1, LINE);
    fill(6, 4, 2, 7, GOLD);
    static const char *const directions[12] = {
        "N", "E", "S", "W", "NE", "SE", "SW", "NW", "U", "D", "I", "O"
    };
    int path_width = 0;
    unsigned back_direction = show_exits ? direction_to_previous_room() : TRAIL_DIRECTIONS;
    if (show_exits) {
        for (unsigned i = 0; i < path_count; i++)
            path_width += direction_label_width(path_options[i], directions) + (i ? 4 : 0);
    }

    int status_width = right && *right ? label_width(right) : 0;
    int name_width = label_width(name);
    bool show_status = status_width != 0;
    // Reserve the same 6px gap that available_name subtracts below, with or without exits,
    // so a shown status never trims the room name.
    if (show_status && name_width + path_width + 6 + status_width + 8 > 221)
        show_status = false;

    int status_x = show_status ? 233 - status_width : 233;
    if (show_status) label(status_x, 4, right, DIM);
    int path_right = show_status ? status_x - 6 : 233;
    int path_x = path_right - path_width;
    int available_name = (show_exits && path_count ? path_x - 6 : path_right - 6) - 12;
    if (available_name < 0) available_name = 0;

    if (show_exits && path_count) {
        int x = path_x;
        for (unsigned i = 0; i < path_count; i++) {
            unsigned direction = path_options[i];
            const uint8_t *exit;
            size_t size = z_property(room_object, path_properties[direction], &exit);
            int width = direction_label_width(direction, directions);
            bool back = direction == back_direction;
            draw_direction_label(x, 4, direction, directions, back && size != 2 ? BACK : size == 2 ? DIM : GOLD);
            if (back && size == 2) fill(x, 12, width, 1, BACK);
            x += width + 4;
        }
    }

    paragraph(12, 4, available_name, 1, name, PAPER);
}

static void header(const char *name, const char *right) {
    draw_header(name, right, false);
}

static void footer(void) {
    fill(0, FOOT_Y, SCREEN_W, SCREEN_H - FOOT_Y, INK);
    fill(0, FOOT_Y, SCREEN_W, 1, LINE);
}

static void key_hint(int x, const char *key, const char *action) {
    int width = label_width(key) + 6;
    box(x, 148, width, 11, PANEL, LINE);
    label(x + 3, 150, key, GOLD);
    label(x + width + 4, 150, action, PAPER);
}

static void text_reset(void) {
    response_size = 0;
    response[0] = 0;
    page_count = page = 0;
    text_truncated = false;
}

static void story_text(const char *s, size_t size) {
    visual_state_observe(chapter, z_global(0), s, size);
    for (size_t i = 0; i < size; i++) {
        char ch = s[i];
        if (ch == '\r') ch = '\n';
        if (ch != '\n' && (unsigned char)ch < 32) ch = ' ';
        if (ch == '\n' && response_size >= 2 && response[response_size - 1] == '\n' &&
            response[response_size - 2] == '\n') continue;
        if (response_size + 32 >= sizeof(response)) {
            text_truncated = true;
            break;
        }
        response[response_size++] = ch;
    }
    response[response_size] = 0;
}

static void set_response(const char *message) {
    text_reset();
    unsigned n = 0;
    while (message[n]) n++;
    story_text(message, n);
}

static void text_pages(void) {
    // A second pass runs only when the first fills every page slot with text left over: the
    // text is cut at the start of the last page and the "omitted" note takes its place.
    for (unsigned pass = 0; pass < 2; pass++) {
        while (response_size && (response[response_size - 1] == '\n' ||
               response[response_size - 1] == ' ' || response[response_size - 1] == '>')) response_size--;
        response[response_size] = 0;
        if (text_truncated) append_text(response, sizeof(response), "\n[End of response omitted]");
        const char *line = response;
        while (*line == '\n') line++;
        page_count = page = 0;
        do {
            page_offsets[page_count++] = (uint16_t)(line - response);
            for (unsigned row = 0; row < STORY_LINES && *line; row++) {
                const char *end;
                line = wrap_line(line, 224, &end);
            }
            while (*line == '\n') line++;
        } while (*line && page_count < sizeof(page_offsets) / sizeof(page_offsets[0]));
        if (!*line) return;
        response_size = page_offsets[page_count - 1];
        text_truncated = true;
    }
}

static void transcript_clear(void) {
    transcript_used = transcript_count = transcript_page = 0;
    transcript_trimmed = false;
    pending_command[0] = 0;
    transcript[0] = 0;
}

static void transcript_load_saved(unsigned size, bool trimmed) {
    if (size > sizeof(transcript)) size = sizeof(transcript);
    if (size && transcript[size - 1] != 0) {
        if (size < sizeof(transcript)) transcript[size++] = 0;
        else transcript[size - 1] = 0;
    }
    if (!size) transcript[0] = 0;
    transcript_count = transcript_page = 0;
    transcript_trimmed = trimmed;
    pending_command[0] = 0;
    // Keep only the newest TRANSCRIPT_ENTRIES non-empty entries, packed from offset 0, so
    // every byte in transcript_used belongs to an indexed entry (discarding relies on it).
    unsigned entries = 0;
    for (unsigned pos = 0; pos < size; pos++) {
        if (transcript[pos] && (pos == 0 || !transcript[pos - 1])) entries++;
    }
    unsigned skip = entries > TRANSCRIPT_ENTRIES ? entries - TRANSCRIPT_ENTRIES : 0;
    if (skip) transcript_trimmed = true;
    unsigned write = 0;
    for (unsigned pos = 0; pos < size;) {
        unsigned start = pos;
        while (pos < size && transcript[pos]) pos++;
        unsigned length = pos - start;
        pos++;
        if (!length) continue;
        if (skip) { skip--; continue; }
        for (unsigned i = 0; i < length; i++) transcript[write + i] = transcript[start + i];
        transcript_start[transcript_count] = (uint16_t)write;
        transcript_rows[transcript_count] = 0;
        transcript_length[transcript_count++] = (uint16_t)length;
        write += length;
        transcript[write++] = 0;
    }
    transcript_used = write;
    if (!write) transcript[0] = 0;
}

static void transcript_discard_oldest(void) {
    if (!transcript_count) return;
    unsigned removed = transcript_length[0] + 1;
    for (unsigned i = removed; i < transcript_used; i++) transcript[i - removed] = transcript[i];
    transcript_used -= removed;
    for (unsigned i = 1; i < transcript_count; i++) {
        transcript_start[i - 1] = transcript_start[i] - removed;
        transcript_length[i - 1] = transcript_length[i];
        transcript_rows[i - 1] = transcript_rows[i];
    }
    transcript_count--;
    transcript_trimmed = true;
}

static void transcript_record_current(void) {
    unsigned command_length = 0;
    while (pending_command[command_length]) command_length++;
    unsigned response_length = 0;
    while (response[response_length]) response_length++;
    if (!command_length && !response_length) {
        pending_command[0] = 0;
        return;
    }

    char command_line[COMMAND_SIZE + 4];
    unsigned command_line_length = 0;
    if (command_length) {
        command_line[command_line_length++] = '>';
        command_line[command_line_length++] = ' ';
        for (unsigned i = 0; i < command_length; i++) command_line[command_line_length++] = pending_command[i];
        command_line[command_line_length++] = '\n';
    }
    // Encode once into the expansion scratch buffer, then copy into the log once room is made.
    uint8_t *encoded = (uint8_t *)transcript_expanded;
    unsigned command_size = 0, response_encoded_size = 0;
    if (command_line_length &&
        !transcript_codec_encode(command_line, command_line_length, encoded,
                                 sizeof(transcript_expanded), &command_size)) {
        pending_command[0] = 0;
        return;
    }
    if (!transcript_codec_encode(response, response_length, encoded + command_size,
                                 sizeof(transcript_expanded) - command_size, &response_encoded_size)) {
        pending_command[0] = 0;
        return;
    }
    unsigned encoded_size = command_size + response_encoded_size;
    unsigned needed = encoded_size + 1;
    if (needed > sizeof(transcript)) {
        pending_command[0] = 0;
        return;
    }
    while (transcript_count == TRANSCRIPT_ENTRIES || transcript_used + needed > sizeof(transcript)) {
        // Every stored byte belongs to an indexed entry; if none remain, start the log afresh
        // instead of looping forever.
        if (!transcript_count) {
            bool trimmed = transcript_used != 0 || transcript_trimmed;
            transcript_used = 0;
            transcript[0] = 0;
            transcript_trimmed = trimmed;
            break;
        }
        transcript_discard_oldest();
    }

    unsigned start = transcript_used;
    for (unsigned i = 0; i < encoded_size; i++) transcript[start + i] = (char)encoded[i];
    transcript_used += encoded_size;
    transcript_length[transcript_count] = (uint16_t)encoded_size;
    transcript_rows[transcript_count] = 0;
    transcript_start[transcript_count++] = (uint16_t)start;
    transcript[transcript_used++] = 0;
    pending_command[0] = 0;
}

static unsigned transcript_entry_rows(unsigned entry) {
    if (transcript_rows[entry]) return transcript_rows[entry];
    unsigned expanded_length = 0;
    if (!transcript_codec_expand((const uint8_t *)transcript + transcript_start[entry], transcript_length[entry],
                                 transcript_expanded, sizeof(transcript_expanded), &expanded_length)) return 1;
    const char *line = transcript_expanded;
    unsigned rows = 0;
    while (*line) {
        const char *end;
        line = wrap_line(line, 224, &end);
        rows++;
    }
    if (!rows) rows = 1;
    transcript_rows[entry] = (uint16_t)rows;
    return rows;
}

static unsigned transcript_page_count(void) {
    unsigned rows = transcript_trimmed ? 1 : 0;
    for (unsigned i = 0; i < transcript_count; i++) {
        rows += transcript_entry_rows(i);
        if (i) rows++;
    }
    if (!rows) rows = 1;
    return (rows + LOG_SCREEN_LINES - 1) / LOG_SCREEN_LINES;
}

static void remove_finished_parser_prompt(void) {
    static const char prompt[] = "Would you like to restart the game from the beginning";
    char *start = (char *)find_substring(response, prompt);
    if (!start) return;
    response_size = (unsigned)(start - response);
    while (response_size && (response[response_size - 1] == '\n' || response[response_size - 1] == '\r' ||
                             response[response_size - 1] == ' '))
        response[--response_size] = 0;
    text_pages();
}

static const char *finished_title(void) {
    if (chapter == 0) return "Zork I complete";
    if (chapter == 1) return "Zork II complete";
    return successful_ending ? "Zork III complete" : "Timeline ended";
}

static void fatal_error(const char *message, unsigned pc, unsigned opcode) {
    fill(0, 0, SCREEN_W, SCREEN_H, PANEL);
    header("Story error", NULL);
    paragraph(8, 27, 224, 3, message, PAPER);
    char detail[48] = "Story address ";
    char digits[8];
    number(digits, pc);
    append_text(detail, sizeof(detail), digits);
    append_text(detail, sizeof(detail), ", opcode ");
    number(digits, opcode);
    append_text(detail, sizeof(detail), digits);
    label(8, 65, detail, DIM);
    footer();
    key_hint(6, "A", "Return to trilogy");
    present();
    while (buttons() & KEY_A) wait_frame();
    while (!(buttons() & KEY_A)) wait_frame();
#ifdef VISUAL_QA
    extern void visual_qa_abort(void) __attribute__((noreturn));
    visual_qa_abort();
#else
    extern void _start(void) __attribute__((noreturn));
    _start();
#endif
}

static uint32_t entropy(void) { return (entropy_state * 747796405u) ^ 29429773u; }
static bool write_progress(void) {
    bool ok = save_write_with_transcript(SRAM, chapter, visited, &trail, &puzzle_map,
                                         (const uint8_t *)transcript, transcript_used, transcript_trimmed);
    if (ok) has_progress = true;
    return ok;
}
static bool restore_progress(void) {
    unsigned saved_transcript_size = 0;
    bool saved_transcript_trimmed = false;
    bool ok = save_read_with_transcript(SRAM, chapter, visited, &trail, &puzzle_map,
                                        (uint8_t *)transcript, sizeof(transcript),
                                        &saved_transcript_size, &saved_transcript_trimmed);
    if (ok) transcript_load_saved(saved_transcript_size, saved_transcript_trimmed);
    restored_snapshot = ok;
    return ok;
}

static void restore_saved_game(void) {
    if (restore_progress()) {
        // save_read restores the cached Royal Puzzle diagram with the story snapshot.
        finished_prompt = successful_ending = false;
        has_progress = true;
        set_response("Saved game restored.");
        busy = true;
        screen = EXPLORE;
    } else {
        set_response("No save could be restored for this chapter.");
        text_pages();
        transcript_record_current();
        if (finished_prompt) {
            screen = FINISHED;
            page = 0;
        } else {
            screen = EXPLORE;
        }
    }
}

static const ZPlatform io = {story_text, fatal_error, entropy, write_progress, restore_progress};

static void refresh_scope(void) {
    unsigned room = z_global(0);
    bool moved = room != room_object;
    room_object = room;
    dark = story_dark(chapter);
    z_name(room, room_name, sizeof(room_name));
    if (!*room_name) append_text(room_name, sizeof(room_name), chapters[chapter].title);
    // A restored snapshot carries its surveyed Royal Puzzle diagram alongside
    // the VM state. Only discard the cache after ordinary travel between rooms.
    if (moved && !restored_snapshot) puzzle_map.valid = false;
    restored_snapshot = false;
    if (chapter == 2 && room == ROYAL_PUZZLE_ROOM) puzzle_map_update(&puzzle_map, response, &response_size);
    if (!dark && room && room < 256) visited[room >> 3] |= 1u << (room & 7);
    unsigned old = focus < nearby.count ? nearby.id[focus] : 0;
    Targets previous_carried = carried;
    story_targets(chapter, &nearby, &carried);
    unsigned dropped_focus = story_focus_dropped_item(&previous_carried, room, &nearby);
    if (dropped_focus < nearby.count) {
        focus = dropped_focus;
        return;
    }
    focus = moved ? 0 : nearby.count;
    if (!moved && old) {
        for (unsigned i = 0; i < nearby.count; i++) if (nearby.id[i] == old) focus = i;
        if (focus == nearby.count && nearby.count) focus = 0;
    }
}

static void begin_story(bool restore) {
    const Chapter *data = &chapters[chapter];
    screen = EXPLORE;
    nearby.count = carried.count = 0;
    focus = room_object = 0;
    travel_room = trail.count = 0;
    puzzle_map.valid = false;
    restored_snapshot = false;
    finished_prompt = successful_ending = false;
    text_reset();
    transcript_clear();
    for (unsigned i = 0; i < VISITED_BYTES; i++) visited[i] = 0;
    visual_state_reset();
    z_start(data->story, (size_t)(data->end - data->story), &io);
    has_progress = restore && restore_progress();
    if (has_progress) set_response("Saved game restored.");
    else if (restore) set_response("Save could not be read. A new game is starting.\n\n");
    busy = true;
}

static void send_command(const char *line) {
    unsigned length = 0;
    while (line[length] && length + 1 < sizeof(pending_command)) {
        pending_command[length] = line[length];
        length++;
    }
    pending_command[length] = 0;
    if (!z_submit(line)) {
        set_response("That command is too long or empty.");
        text_pages();
        transcript_record_current();
        screen = EXPLORE;
        return;
    }
    travel_room = 0;
    if (!dark) {
        for (unsigned i = 0; i < TRAIL_DIRECTIONS; i++) {
            if (text_equal(line, path_commands[i]) || text_equal(line, path_names[i])) {
                travel_room = room_object;
                travel_direction = i;
                break;
            }
        }
    }
    text_reset();
    has_progress = false;
    finished_prompt = successful_ending = false;
    screen = EXPLORE;
    busy = true;
}

static void complete_turn(void) {
    ZResult result = z_run(6000);
    if (result == Z_BUSY) return;
    busy = false;
    if (travel_room && !story_dark(chapter)) trail_record(&trail, travel_room, travel_direction, z_global(0));
    travel_room = 0;
    refresh_scope();
    text_pages();
    if (result == Z_ENDED) screen = FINISHED;
    finished_prompt = text_has(response, "Would you like to restart the game from the beginning");
    successful_ending = chapter == 2 && finished_prompt && text_has(response, "Your potential is");
    if (finished_prompt) {
        remove_finished_parser_prompt();
        screen = FINISHED;
    }
    transcript_record_current();
}

static bool room_seen(unsigned id) {
    return id && id < VISITED_BYTES * 8 && (visited[id >> 3] & (1u << (id & 7)));
}

static void visible_places(void) {
    place_count = 0;
    unsigned count = z_object_count();
    char candidate[Z_NAME_MAX];
    for (unsigned id = 1; id <= count; id++) {
        if (!room_seen(id)) continue;
        z_name(id, candidate, sizeof(candidate));
        if (*candidate) places[place_count++] = id;
    }
}

static unsigned map_exit(unsigned direction) {
    if (direction >= TRAIL_DIRECTIONS || !map_room) return 0;
    unsigned room = trail_destination(&trail, map_room, direction);
    if (room_seen(room)) return room;
    const uint8_t *exit;
    if (z_property(map_room, path_properties[direction], &exit) == 1 && room_seen(exit[0])) return exit[0];
    return 0;
}

static void draw_map(void) {
    header("Explored rooms", map_vertical ? "Up / Down / In / Out" : "Compass");
    uint8_t rooms[MAP_CELLS];
    for (unsigned i = 0; i < MAP_CELLS; i++) {
        rooms[i] = i == MAP_CENTER ? map_room : map_exit(map_direction(map_vertical, i));
    }
    draw_room_map(rooms, map_vertical, map_cursor, room_object);
    footer();
    key_hint(5, "PAD", "Move");
    key_hint(59, "A", "View");
    key_hint(97, "L/R", "Tab");
    key_hint(142, "SEL", "Rooms");
    key_hint(199, "B", "Back");
}

static void open_map(void) {
    visible_places();
    map_room = room_seen(room_object) ? room_object : place_count ? places[0] : 0;
    map_cursor = MAP_CENTER;
    map_vertical = false;
    screen = MAP;
}

static void map_pressed(uint16_t pressed) {
    if (pressed & (KEY_B | KEY_START)) { screen = PAUSE; selected = 4; }
    else if (pressed & KEY_SELECT) {
        screen = PLACES;
        selected = 0;
        for (unsigned i = 0; i < place_count; i++) if (places[i] == map_room) selected = i;
    } else if (pressed & (KEY_L | KEY_R)) {
        map_vertical = !map_vertical;
        map_cursor = MAP_CENTER;
    } else if ((pressed & KEY_UP) && map_cursor >= 3) map_cursor -= 3;
    else if ((pressed & KEY_DOWN) && map_cursor < 6) map_cursor += 3;
    else if ((pressed & KEY_LEFT) && map_cursor != 0 && map_cursor != 3 && map_cursor != 6) map_cursor--;
    else if ((pressed & KEY_RIGHT) && map_cursor != 2 && map_cursor != 5 && map_cursor != 8) map_cursor++;
    else if (pressed & KEY_A) {
        unsigned next = map_exit(map_direction(map_vertical, map_cursor));
        if (next) {
            map_room = next;
            map_cursor = MAP_CENTER;
        }
    }
}

static void draw_explore(bool reviewing) {
    char status[20], moves[8];
    number(status, (int16_t)z_global(1));
    number(moves, z_global(2));
    append_text(status, sizeof(status), " / ");
    append_text(status, sizeof(status), moves);
    bool barrow_ending = chapter == 0 && zork1_barrow_finished();
    const char *title = finished_prompt ? finished_title() :
                        (barrow_ending ? "Inside the Barrow" : room_name);
    bool show_exits = !barrow_ending && screen != FINISHED;
    if (show_exits) collect_paths();
    draw_header(dark ? "Darkness" : title, status, show_exits);

    unsigned focused = 0;
    if (!reviewing && !barrow_ending) {
        unsigned index = screen == NEARBY ? selected : focus;
        if (index < nearby.count) focused = nearby.id[index];
    }

    if (dark) {
        fill(0, ART_Y, SCREEN_W, ART_HEIGHT, BLACK);
        if (chapter == 1 && room_object == 125)
            draw_room_details(chapter, room_object, story_scene(chapter, room_name), &nearby, focused);
    }
    else if (chapter == 2 && room_object == ROYAL_PUZZLE_ROOM)
        draw_puzzle_map(&puzzle_map, &nearby, focused);
    else {
        const Color *art = story_scene(chapter, room_name);
        scene_rows(ART_Y, art + 10 * SCREEN_W, ART_HEIGHT);
        if (!barrow_ending) draw_room_details(chapter, room_object, art, &nearby, focused);
    }
    if (screen == FINISHED && chapter == 2 && room_object == ZORK3_TREASURY && successful_ending)
        draw_z3_ending_cue(scene_animation_stage());

    fill(0, TEXT_Y, SCREEN_W, FOOT_Y - TEXT_Y, PANEL);
    fill(0, TEXT_Y, SCREEN_W, 1, LINE);
    if (!reviewing && !barrow_ending && screen != FINISHED) {
        char item[Z_NAME_MAX] = "Room actions";
        if (focused) {
            story_name(chapter, focused, item);
            if (chapter == 2 && room_object == 45 && focused == 11 && !z_attr(183, ATTR_OPEN)) {
                item[0] = 0;
                append_text(item, sizeof(item), "Dungeon Master (outside)");
            }
        }
        int text_x = focused ? 22 : 8;
        fill(0, TEXT_Y + 1, SCREEN_W, 11, INK);
        draw_target_focus_box(true);
        if (focused) draw_story_icon(chapter, focused, 8, TEXT_Y + 2);
        paragraph(text_x, TEXT_Y + 3, 232 - text_x, 1, item, GOLD);
    }
    int story_y = reviewing || barrow_ending || screen == FINISHED ? TEXT_Y + 5 : TEXT_Y + 15;
    if (busy) label(8, story_y, "One moment...", DIM);
    else paragraph(8, story_y, 224, STORY_LINES, response + page_offsets[page], PAPER);
    footer();
    if (reviewing) {
        key_hint(6, "L/R", "Page");
        key_hint(90, "B", "Back");
    } else if (page + 1 < page_count) {
        key_hint(6, "A", "Read more");
        key_hint(88, "B", "Skip to end");
    } else if (screen == FINISHED) {
        if (finished_prompt) {
            key_hint(6, "A", "Options");
            key_hint(83, "L/R", "Page");
        } else {
            key_hint(6, "A", "Trilogy menu");
            key_hint(106, "L/R", "Page");
        }
    } else {
        key_hint(6, "UP", "Paths");
        key_hint(57, "A", "Act");
        key_hint(92, "B", "Look");
        key_hint(132, "L/R", "Focus");
        key_hint(187, "SEL", "Bag");
    }
}

static void draw_transcript_row(const char *start, const char *end, int y) {
    char line[225];
    unsigned length = (unsigned)(end - start);
    if (length > sizeof(line) - 1) length = sizeof(line) - 1;
    for (unsigned i = 0; i < length; i++) line[i] = start[i];
    line[length] = 0;
    label(8, y, line, PAPER);
}

static void draw_story_log(void) {
    unsigned pages = transcript_page_count();
    if (transcript_page >= pages) transcript_page = pages - 1;
    char status[16], total[8];
    number(status, (int)transcript_page + 1);
    append_text(status, sizeof(status), " / ");
    number(total, pages);
    append_text(status, sizeof(status), total);
    header("Story so far", status);
    fill(0, ART_Y, SCREEN_W, FOOT_Y - ART_Y, PANEL);
    label(8, 19, "Most recent first", DIM);

    unsigned skip = transcript_page * LOG_SCREEN_LINES, row = 0;
    int y = 30;
    for (unsigned item = transcript_count; item > 0 && row < LOG_SCREEN_LINES; item--) {
        // Entries wholly before this page are skipped by their cached row count, unexpanded.
        unsigned entry_rows = transcript_entry_rows(item - 1) + (item > 1 ? 1 : 0);
        if (skip >= entry_rows) {
            skip -= entry_rows;
            continue;
        }
        unsigned expanded_length = 0;
        if (!transcript_codec_expand((const uint8_t *)transcript + transcript_start[item - 1],
                                     transcript_length[item - 1], transcript_expanded,
                                     sizeof(transcript_expanded), &expanded_length)) {
            if (!skip) {
                label(8, y, "[Story text could not be expanded]", PAPER);
                y += 10;
                row++;
            } else skip--;
            continue;
        }
        const char *line = transcript_expanded;
        while (*line && row < LOG_SCREEN_LINES) {
            const char *end;
            const char *next = wrap_line(line, 224, &end);
            if (skip) skip--;
            else {
                draw_transcript_row(line, end, y);
                y += 10;
                row++;
            }
            line = next;
        }
        if (item > 1 && row < LOG_SCREEN_LINES) {
            if (skip) skip--;
            else {
                y += 10;
                row++;
            }
        }
    }
    if (transcript_trimmed && row < LOG_SCREEN_LINES) {
        if (skip) skip--;
        else label(8, y, "Earlier text trimmed.", DIM);
    }
    if (!transcript_count) label(8, 30, "No story text yet.", DIM);

    footer();
    key_hint(6, "L", "Newer");
    key_hint(67, "R", "Older");
    key_hint(147, "B", "Back");
}

static void draw_finished_menu(void) {
    char status[20], moves[8];
    number(status, (int16_t)z_global(1));
    number(moves, z_global(2));
    append_text(status, sizeof(status), " / ");
    append_text(status, sizeof(status), moves);
    draw_header(finished_title(), status, false);

    if (dark) fill(0, ART_Y, SCREEN_W, ART_HEIGHT, BLACK);
    else {
        const Color *art = story_scene(chapter, room_name);
        scene_rows(ART_Y, art + 10 * SCREEN_W, ART_HEIGHT);
        if (!(chapter == 0 && zork1_barrow_finished()))
            draw_room_details(chapter, room_object, art, &nearby, 0);
    }
    if (chapter == 2 && room_object == ZORK3_TREASURY && successful_ending)
        draw_z3_ending_cue(2);

    fill(0, TEXT_Y, SCREEN_W, FOOT_Y - TEXT_Y, PANEL);
    fill(0, TEXT_Y, SCREEN_W, 1, LINE);
    paragraph(8, TEXT_Y + 3, 224, 1, "Choose what to do next", GOLD);
    fill(8, TEXT_Y + 13, SCREEN_W - 16, 1, LINE);
    for (unsigned i = 0; i < sizeof(finished_options) / sizeof(finished_options[0]); i++) {
        int y = TEXT_Y + 16 + (int)i * 10;
        if (i == selected) {
            fill(7, y, 226, 10, INK);
            fill(8, y + 2, 2, 6, GOLD);
        }
        paragraph(14, y + 1, 216, 1, finished_options[i], i == selected ? PAPER : DIM);
    }
    footer();
    key_hint(6, "PAD", "Choose");
    key_hint(83, "A", "Confirm");
    key_hint(159, "B", "Back");
}

static unsigned view_start(unsigned total) {
    if (total <= MENU_LINES || selected < MENU_LINES / 2) return 0;
    unsigned first = selected - MENU_LINES / 2;
    return first + MENU_LINES > total ? total - MENU_LINES : first;
}

static void open_menu(Screen next) {
    screen = next;
    selected = 0;
}

static void return_to(MenuCursor saved) {
    screen = saved.screen;
    selected = saved.row;
}

static void collect_paths(void) {
    path_count = 0;
    const uint8_t *exit;

    // The Zork II angular maze accepts every direction and reorients passages.
    if (chapter == 1 && text_equal(room_name, "Oddly-angled Room")) {
        for (unsigned direction = 0; direction < 12; direction++)
            path_options[path_count++] = direction;
        return;
    }

    static const uint8_t puzzle_neighbor[8] = {1, 5, 7, 3, 2, 8, 6, 0};
    for (unsigned direction = 0; direction < 12; direction++) {
        size_t size = z_property(room_object, path_properties[direction], &exit);
        // Exit routines can still reject a candidate based on story state.
        if (chapter == 2 && room_object == ROYAL_PUZZLE_ROOM && puzzle_map.valid &&
            direction < 8 && puzzle_map.tiles[puzzle_neighbor[direction]] == PUZZLE_UNKNOWN)
            continue;
        if (size) {
            path_options[path_count++] = direction;
        } else if (chapter == 1 && text_equal(room_name, "Carousel Room") &&
                   (direction == 10 || direction == 11)) {
            // The Carousel's In/Out commands choose a random passage in the story.
            path_options[path_count++] = direction;
        }
    }
}

static void open_room_menu(Screen next) {
    room_menu_origin = (MenuCursor){screen, selected};
    if (next == PATHS) collect_paths();
    open_menu(next);
}

static void drawer_frame(const char *name, unsigned count, unsigned first) {
    fill(0, TEXT_Y, SCREEN_W, FOOT_Y - TEXT_Y, INK);
    fill(0, TEXT_Y, SCREEN_W, 1, LINE);
    paragraph(8, TEXT_Y + 3, 208, 1, name, GOLD);
    fill(8, TEXT_Y + 12, SCREEN_W - 16, 1, LINE);
    if (first) label(217, TEXT_Y + 3, "^", DIM);
    if (first + MENU_LINES < count) label(227, TEXT_Y + 3, "+", GOLD);
    footer();
    key_hint(6, "PAD", "Choose");
    key_hint(83, "A", "Confirm");
    key_hint(159, "B", "Back");
}

static void drawer_row(unsigned first, unsigned i, const char *name, bool active, unsigned object) {
    int y = TEXT_Y + 14 + (i - first) * 10;
    if (i == selected) {
        fill(7, y, 226, 10, PANEL);
        fill(8, y + 2, 2, 6, GOLD);
    }
    int text_x = 14;
    if (object) {
        draw_story_icon(chapter, object, text_x, y);
        text_x += STORY_ICON_SIZE + 4;
    }
    paragraph(text_x, y + 1, 231 - text_x, 1, name, active ? (i == selected ? PAPER : DIM) : LINE);
}

static unsigned list_count(void) {
    switch (screen) {
        case SLOT: return 3;
        case BAG: return carried.count;
        case NEARBY: return nearby.count;
        case VERBS:
        case GROUP:
        case ORDERS_MENU: return action_count;
        case CATEGORIES: return sizeof(category_options) / sizeof(category_options[0]);
        case SECOND_OBJECT: return second.count;
        case HEADING: return 12;
        case SPEAK: return sizeof(speech_options) / sizeof(speech_options[0]);
        case ROOM_ACTIONS: return sizeof(room_options) / sizeof(room_options[0]);
        case PATHS: return path_count;
        case PAUSE: return sizeof(pause_options) / sizeof(pause_options[0]);
        case PLACES: return place_count;
        case RETURN_CONFIRM: return 2;
        default: return 0;
    }
}

static void draw_drawer(void) {
    char object[Z_NAME_MAX], item[Z_NAME_MAX];
    const char *title_text = "";
    unsigned count = list_count(), first = view_start(count);
    switch (screen) {
        case BAG: title_text = "Carried items"; break;
        case NEARBY: title_text = "Nearby objects"; break;
        case VERBS:
        case GROUP: story_name(chapter, action_object, object); title_text = object; break;
        case CATEGORIES: title_text = "More actions"; break;
        case ORDERS_MENU: title_text = "Give an order"; break;
        case SECOND_OBJECT: title_text = pending_action.name; break;
        case HEADING: title_text = "Choose a direction"; break;
        case SPEAK: title_text = "Speak"; break;
        case ROOM_ACTIONS: title_text = "Room actions"; break;
        case PATHS: title_text = "Paths"; break;
        case PAUSE: title_text = "Paused"; break;
        case PLACES: title_text = "Places visited"; break;
        case RETURN_CONFIRM: title_text = "Leave this game?"; break;
        default: break;
    }
    drawer_frame(title_text, count, first);
    if (!count) {
        const char *empty = "Nothing nearby.";
        if (screen == BAG) empty = "Nothing carried.";
        if (screen == PLACES) empty = "No places seen yet.";
        if (screen == PATHS) empty = "No paths from here.";
        label(14, TEXT_Y + 16, empty, DIM);
    }
    for (unsigned i = first; i < count && i < first + MENU_LINES; i++) {
        const char *name = item;
        unsigned id = 0;
        switch (screen) {
            case BAG: id = carried.id[i]; break;
            case NEARBY: id = nearby.id[i]; break;
            case VERBS:
            case GROUP:
            case ORDERS_MENU: name = action_list[i].name; break;
            case CATEGORIES: name = category_options[i]; break;
            case SECOND_OBJECT: id = second.id[i]; break;
            case HEADING: name = path_names[i]; break;
            case SPEAK: name = speech_options[i]; break;
            case ROOM_ACTIONS: name = room_options[i].name; break;
            case PATHS: name = path_names[path_options[i]]; break;
            case PAUSE: name = pause_options[i]; break;
            case PLACES: z_name(places[i], item, sizeof(item)); break;
            case RETURN_CONFIRM: name = return_options[i]; break;
            default: name = "";
        }
        if (id) story_name(chapter, id, item);
        drawer_row(first, i, name, true, id);
    }
}

static void draw_launcher(void) {
    bool slot = screen == SLOT;
    header(slot ? chapters[chapter].title : "INFOCOM ZORK TRILOGY", NULL);
    scene_rows(ART_Y, chapters[chapter].opening + 17 * SCREEN_W, 64);
    fill(0, 79, SCREEN_W, FOOT_Y - 79, PANEL);
    if (!slot) {
        box(6, 22, 77, 30, INK, LINE);
        title(12, 27, "ZORK", GOLD, 3);
        for (unsigned i = 0; i < CHAPTERS; i++) {
            int y = 81 + i * 21;
            if (i == chapter) {
                fill(3, y, 234, 20, INK);
                fill(6, y + 3, 2, 13, GOLD);
            }
            label(13, y + 3, chapters[i].title, i == chapter ? GOLD : DIM);
            label(70, y + 3, chapters[i].subtitle, PAPER);
            if (save_exists(SRAM, i)) label(70, y + 12, "Saved game", DIM);
        }
        footer();
        key_hint(6, "PAD", "Chapter");
        key_hint(89, "A", "Select");
    } else {
        bool saved = save_exists(SRAM, chapter);
        drawer_frame("Play chapter", 3, 0);
        for (unsigned i = 0; i < 3; i++) drawer_row(0, i, slot_options[i], saved || i != 0, 0);
    }
}

static void draw_controls(void) {
    header("Controls", NULL);
    fill(0, ART_Y, SCREEN_W, FOOT_Y - ART_Y, PANEL);
    paragraph(8, 24, 224, 14,
        "Up in a room: Open Paths.\n"
        "In Paths: D-pad chooses; A travels.\n"
        "Paths lists exits here.\n"
        "Right triangles show diagonals.\n"
        "Teal marks a known way back.\n"
        "Dim routes may need a condition.\n"
        "L / R: Focus on object or room.\n"
        "A: Read, act, or confirm.\n"
        "B: Look or go back.\n"
        "Select: Open your bag.\n"
        "Start: Pause menu has Story so far.\n\n"
        "Room actions: Nearby objects,\n"
        "speech, and other commands.", PAPER);
    footer();
    key_hint(6, "B", "Back");
}

static void draw_keyboard(void) {
    header("Enter command", NULL);
    fill(0, ART_Y, SCREEN_W, FOOT_Y - ART_Y, PANEL);
    box(5, 22, 230, 32, INK, LINE);
    paragraph(10, 27, 218, 2, *command ? command : "Choose letters. Press Start to send.", *command ? PAPER : DIM);
    for (unsigned i = 0; i < sizeof(keys) - 1; i++) {
        int x = 7 + (i % 10) * 23, y = 60 + (i / 10) * 16;
        bool active = i == key_cursor;
        box(x, y, 18, 13, active ? GOLD : INK, active ? GOLD : LINE);
        char key[2] = {keys[i], 0};
        label(x + 7, y + 3, key, active ? INK : PAPER);
    }
    footer();
    key_hint(6, "R", "Space");
    key_hint(53, "B", "Erase");
    key_hint(101, "SEL", "Back");
    key_hint(166, "START", "Send");
}

static void phrase_text(char phrase[WORDS_PER_PHRASE * Z_WORD_MAX]) {
    phrase[0] = 0;
    for (unsigned i = 0; i < spoken_count; i++) {
        char word[Z_WORD_MAX];
        speech_word(spoken[i], word);
        if (i) append_text(phrase, WORDS_PER_PHRASE * Z_WORD_MAX, " ");
        append_text(phrase, WORDS_PER_PHRASE * Z_WORD_MAX, word);
    }
}

enum { LETTER_COLUMNS = 9, WORD_ROWS = 6, WORD_PAGE = WORD_ROWS * 2 };

static void draw_speech(void) {
    char phrase[WORDS_PER_PHRASE * Z_WORD_MAX];
    phrase_text(phrase);
    header(speech_titles[speech_mode], NULL);
    fill(0, ART_Y, SCREEN_W, FOOT_Y - ART_Y, PANEL);
    box(6, 22, 228, 23, INK, LINE);
    paragraph(12, 29, 216, 1, *phrase ? phrase : "Choose words, then press Start.", *phrase ? PAPER : DIM);
    if (screen == LETTERS) {
        label(8, 54, spoken_count == WORDS_PER_PHRASE ? "Phrase full. Send or erase a word." : "First letter", DIM);
        for (unsigned i = 0; i < 26; i++) {
            int x = 8 + (i % LETTER_COLUMNS) * 25, y = 68 + (i / LETTER_COLUMNS) * 23;
            bool active = letter_cursor == i;
            box(x, y, 20, 17, active ? GOLD : INK, active ? GOLD : LINE);
            char letter[] = {(char)('A' + i), 0};
            label(x + 8, y + 5, letter, active ? INK : PAPER);
        }
    } else {
        unsigned first = word_cursor - word_cursor % WORD_PAGE;
        char letter[] = {(char)('A' + letter_cursor), 0};
        label(8, 50, letter, GOLD);
        if (word_count > WORD_PAGE) label(25, 50, "L/R  More words", DIM);
        for (unsigned i = first; i < word_count && i < first + WORD_PAGE; i++) {
            int x = 8 + ((i - first) / WORD_ROWS) * 113, y = 62 + ((i - first) % WORD_ROWS) * 13;
            char word[Z_WORD_MAX];
            speech_word(words[i], word);
            fill(x, y, 107, 12, i == word_cursor ? GOLD : INK);
            label(x + 5, y + 2, word, i == word_cursor ? INK : PAPER);
        }
        if (!word_count) label(8, 70, "No words under this letter.", DIM);
    }
    footer();
    key_hint(6, "A", screen == LETTERS ? "Choose" : "Add");
    key_hint(55, "B", "Back");
    key_hint(100, "SEL", "Erase");
    key_hint(177, "START", "Send");
}

static void draw_dial(void) {
    header(pending_action.name, NULL);
    fill(0, ART_Y, SCREEN_W, FOOT_Y - ART_Y, PANEL);
    bool year = dial_digits > 1;
    label(12, 33, year ? "Choose a year" : "Choose a dial position", PAPER);
    label(12, 119, year ? "Up/down changes a digit. Left/right moves." : "Up/down changes the position.", DIM);
    for (unsigned i = 0; i < dial_digits; i++) {
        bool active = i == dial_cursor;
        int x = 106 - (year ? 36 : 0) + i * 36;
        box(x, 62, 28, 40, active ? GOLD : INK, active ? GOLD : LINE);
        char digit[] = {(char)('0' + digits[i]), 0};
        title(x + 10, 75, digit, active ? INK : PAPER, 2);
    }
    footer();
    key_hint(6, "PAD", "Adjust");
    key_hint(83, "A", "Confirm");
    key_hint(159, "B", "Back");
}

static void render(void) {
    if (screen == LAUNCHER || screen == SLOT) draw_launcher();
    else if (screen == COMMAND) draw_keyboard();
    else if (screen == CONTROLS) draw_controls();
    else if (screen == MAP) draw_map();
    else if (screen == STORY_LOG) draw_story_log();
    else if (screen == LETTERS || screen == WORDS) draw_speech();
    else if (screen == DIAL) draw_dial();
    else if (screen == FINISHED_MENU) draw_finished_menu();
    else {
        draw_explore(screen == TEXT_REVIEW);
        if (screen != EXPLORE && screen != TEXT_REVIEW && screen != FINISHED) draw_drawer();
    }
}

#ifdef VISUAL_QA
// Host replay hook: exercise the real render() -> draw_explore() path for one
// currently available target, including the target name row and its focus box.
bool visual_qa_render_focus_frame(unsigned chapter_id, unsigned room, unsigned object) {
    if (chapter_id >= sizeof(chapters) / sizeof(chapters[0]) || z_global(0) != room) return false;
    chapter = chapter_id;
    screen = EXPLORE;
    room_object = room;
    dark = story_dark(chapter);
    finished_prompt = successful_ending = false;
    busy = false;
    page = 0;
    page_count = 1;
    page_offsets[0] = 0;
    response[0] = 0;
    response_size = 0;
    z_name(room, room_name, sizeof(room_name));
    story_targets(chapter, &nearby, &carried);
    focus = nearby.count;
    for (unsigned i = 0; i < nearby.count; i++) {
        if (nearby.id[i] == object) {
            focus = i;
            break;
        }
    }
    if (focus == nearby.count) return false;
    render();
    return true;
}
#endif

static void rotate(unsigned *index, unsigned count, bool forward) {
    if (!count) return;
    if (forward) *index = *index + 1 == count ? 0 : *index + 1;
    else *index = *index ? *index - 1 : count - 1;
}

static void choose_action_list(Screen next, ActionGroup group) {
    open_menu(next);
    if (group == PRIMARY) {
        story_menu_probe_begin();
        story_menu_probe_set_budget(STORY_MENU_PROBE_BUDGET);
        action_count = story_actions(chapter, action_object, verb_is_bag, group, action_list);
        action_count = story_filter_defaults_masked(chapter, action_object, action_list, action_count);
        story_menu_probe_end();
    } else action_count = story_actions_fast(chapter, action_object, verb_is_bag, group, action_list);
}

static void pick_object(unsigned object) {
    action_object = object;
    object_origin = (MenuCursor){screen, selected};
    verb_is_bag = z_parent(object) == chapters[chapter].player;
    choose_action_list(VERBS, PRIMARY);
}

static void open_command(Screen back) {
    command_back = back;
    command[0] = 0;
    key_cursor = 0;
    screen = COMMAND;
}

static void run_action(Action action, const char *value) {
    char line[COMMAND_SIZE];
    if (action_command(action, action_object, value, line)) send_command(line);
}

static void add_target(unsigned id) {
    if (id == action_object || second.count == TARGET_LIMIT) return;
    for (unsigned i = 0; i < second.count; i++) if (second.id[i] == id) return;
    second.id[second.count++] = id;
}

static void collect_second(Action action) {
    second.count = 0;
    if (action.kind == PICK_ITEM) {
        for (unsigned i = 0; i < carried.count; i++) add_target(carried.id[i]);
        return;
    }
    if (action.kind == PICK_SELF) add_target(TARGET_SELF);
    bool tools_first = action.argument && text_equal(action.argument, "with");
    const Targets *first = tools_first ? &carried : &nearby;
    const Targets *last = tools_first ? &nearby : &carried;
    for (unsigned i = 0; i < first->count; i++) add_target(first->id[i]);
    for (unsigned i = 0; i < last->count; i++) add_target(last->id[i]);
}

static void handle_verb(void) {
    Action action = action_list[selected];
    MenuCursor origin = {screen, selected};
    switch (action.kind) {
        case SHOW_CATEGORIES:
        case SHOW_ORDERS:
            category_origin = origin;
            if (action.kind == SHOW_ORDERS) choose_action_list(ORDERS_MENU, ORDERS);
            else open_menu(CATEGORIES);
            break;
        case DO_OBJECT:
        case DO_LITERAL: run_action(action, NULL); break;
        default:
            argument_origin = origin;
            pending_action = action;
            if (action.kind == PICK_TARGET || action.kind == PICK_SELF || action.kind == PICK_ITEM) {
                collect_second(action);
                open_menu(SECOND_OBJECT);
            } else if (action.kind == PICK_DIRECTION) open_menu(HEADING);
            else {
                dial_digits = action.kind == PICK_YEAR ? sizeof(digits) : 1;
                for (unsigned i = 0; i < sizeof(digits); i++) digits[i] = 0;
                digits[0] = dial_digits == 1 ? 1 : 0;
                dial_cursor = 0;
                open_menu(DIAL);
            }
    }
}

static void play_speech(unsigned mode) {
    speech_mode = mode;
    speech_origin = (MenuCursor){SPEAK, selected};
    letter_cursor = spoken_count = 0;
    screen = LETTERS;
}

static void collect_words(void) {
    word_cursor = word_count = 0;
    unsigned size = z_dictionary_size();
    for (unsigned i = 0; i < size && word_count < sizeof(words) / sizeof(words[0]); i++) {
        char word[Z_WORD_MAX];
        speech_word(i, word);
        if (word[0] == 'a' + (int)letter_cursor) words[word_count++] = i;
    }
    screen = WORDS;
}

static void speech_pressed(uint16_t pressed) {
    if (pressed & KEY_START) {
        char phrase[WORDS_PER_PHRASE * Z_WORD_MAX], line[COMMAND_SIZE];
        phrase_text(phrase);
        if (speech_command(speech_mode, phrase, line)) send_command(line);
    } else if (pressed & KEY_B) {
        if (screen == WORDS) screen = LETTERS;
        else return_to(speech_origin);
    } else if (pressed & KEY_SELECT) {
        if (spoken_count) spoken_count--;
    } else if (screen == LETTERS) {
        if (pressed & KEY_LEFT) rotate(&letter_cursor, 26, false);
        else if (pressed & KEY_RIGHT) rotate(&letter_cursor, 26, true);
        else if ((pressed & KEY_UP) && letter_cursor >= LETTER_COLUMNS) letter_cursor -= LETTER_COLUMNS;
        else if ((pressed & KEY_DOWN) && letter_cursor + LETTER_COLUMNS < 26) letter_cursor += LETTER_COLUMNS;
        else if ((pressed & KEY_A) && spoken_count < WORDS_PER_PHRASE) collect_words();
    } else if (word_count) {
        if (pressed & KEY_UP) rotate(&word_cursor, word_count, false);
        else if (pressed & KEY_DOWN) rotate(&word_cursor, word_count, true);
        else if ((pressed & KEY_LEFT) && word_cursor >= WORD_ROWS) word_cursor -= WORD_ROWS;
        else if ((pressed & KEY_RIGHT) && word_cursor + WORD_ROWS < word_count) word_cursor += WORD_ROWS;
        else if (pressed & (KEY_L | KEY_R)) {
            unsigned pages = (word_count + WORD_PAGE - 1) / WORD_PAGE;
            unsigned current = word_cursor / WORD_PAGE;
            rotate(&current, pages, pressed & KEY_R);
            word_cursor = current * WORD_PAGE;
        } else if (pressed & KEY_A) {
            spoken[spoken_count++] = words[word_cursor];
            screen = LETTERS;
        }
    }
}

static void dial_pressed(uint16_t pressed) {
    bool year = dial_digits > 1;
    if (pressed & KEY_B) return_to(argument_origin);
    else if (pressed & KEY_LEFT) rotate(&dial_cursor, dial_digits, false);
    else if (pressed & KEY_RIGHT) rotate(&dial_cursor, dial_digits, true);
    else if (pressed & (KEY_UP | KEY_DOWN)) {
        unsigned minimum = year ? 0 : 1, count = year ? 10 : 8;
        unsigned value = digits[dial_cursor] - minimum;
        rotate(&value, count, pressed & KEY_UP);
        digits[dial_cursor] = value + minimum;
    } else if (pressed & KEY_A) {
        unsigned value = 0;
        for (unsigned i = 0; i < dial_digits; i++) value = value * 10 + digits[i];
        char out[8];
        number(out, value);
        run_action(pending_action, out);
    }
}

static void handle_pause(void) {
    switch (selected) {
        case 0: screen = EXPLORE; break;
        case 1: transcript_page = 0; screen = STORY_LOG; break;
        case 2:
            set_response(write_progress() ? "Game saved." : "Could not save this game.");
            text_pages();
            transcript_record_current();
            screen = EXPLORE;
            break;
        case 3:
            restore_saved_game();
            break;
        case 4: open_map(); break;
        case 5: screen = TEXT_REVIEW; page = 0; break;
        case 6: open_command(PAUSE); break;
        case 7: screen = CONTROLS; break;
        case 8:
            if (has_progress) screen = LAUNCHER;
            else { screen = RETURN_CONFIRM; selected = 0; }
            break;
    }
}

static void explore_pressed(uint16_t pressed) {
    if (pressed & KEY_START) {
        screen = PAUSE;
        selected = 0;
    } else if (page + 1 < page_count && (pressed & (KEY_A | KEY_B))) {
        page = (pressed & KEY_B) ? page_count - 1 : page + 1;
    } else if (pressed & KEY_SELECT) {
        selected = 0;
        screen = BAG;
    } else if (pressed & KEY_UP) {
        open_room_menu(PATHS);
    } else if (pressed & KEY_R) rotate(&focus, nearby.count + 1, true);
    else if (pressed & KEY_L) rotate(&focus, nearby.count + 1, false);
    else if (pressed & KEY_A) {
        if (focus < nearby.count) pick_object(nearby.id[focus]);
        else { screen = ROOM_ACTIONS; selected = 0; }
    } else if (pressed & KEY_B) send_command("look");
}

static void keyboard_pressed(uint16_t pressed) {
    const unsigned count = sizeof(keys) - 1;
    if (pressed & KEY_SELECT) screen = command_back;
    else if (pressed & KEY_START) send_command(command);
    else if (pressed & KEY_LEFT) rotate(&key_cursor, count, false);
    else if (pressed & KEY_RIGHT) rotate(&key_cursor, count, true);
    else if (pressed & KEY_UP) key_cursor = key_cursor >= 10 ? key_cursor - 10 : key_cursor;
    else if (pressed & KEY_DOWN) key_cursor = key_cursor + 10 < count ? key_cursor + 10 : key_cursor;
    else if (pressed & KEY_B) {
        unsigned p = 0;
        while (command[p]) p++;
        if (p) command[p - 1] = 0;
    } else if (pressed & (KEY_A | KEY_R)) {
        char key[2] = {pressed & KEY_R ? ' ' : keys[key_cursor], 0};
        unsigned capacity = z_input_limit() + 1;
        if (capacity > sizeof(command)) capacity = sizeof(command);
        append_text(command, capacity, key);
    }
}

static void back_out(void) {
    switch (screen) {
        case SLOT: screen = LAUNCHER; break;
        case VERBS: return_to(object_origin); break;
        case ORDERS_MENU:
        case CATEGORIES:
            choose_action_list(VERBS, PRIMARY);
            return_to(category_origin);
            break;
        case GROUP: screen = CATEGORIES; selected = category_row; break;
        case SECOND_OBJECT:
        case HEADING: return_to(argument_origin); break;
        case SPEAK:
        case PATHS:
        case NEARBY: return_to(room_menu_origin); break;
        case PLACES: screen = MAP; break;
        case STORY_LOG: screen = PAUSE; selected = 1; break;
        case CONTROLS: screen = PAUSE; selected = 7; break;
        case TEXT_REVIEW: screen = PAUSE; selected = 5; break;
        case RETURN_CONFIRM: screen = PAUSE; selected = 8; break;
        case FINISHED_MENU:
            screen = FINISHED;
            page = page_count ? page_count - 1 : 0;
            break;
        default: screen = EXPLORE; break;
    }
}

static void update(uint16_t pressed) {
    if (screen == LAUNCHER) {
        if (pressed & (KEY_UP | KEY_LEFT)) rotate(&chapter, CHAPTERS, false);
        else if (pressed & (KEY_DOWN | KEY_RIGHT)) rotate(&chapter, CHAPTERS, true);
        else if (pressed & (KEY_A | KEY_START)) {
            selected = save_exists(SRAM, chapter) ? 0 : 1;
            screen = SLOT;
        }
        return;
    }
    if (screen == EXPLORE) { explore_pressed(pressed); return; }
    if (screen == MAP) { map_pressed(pressed); return; }
    if (screen == STORY_LOG) {
        if (pressed & KEY_B) back_out();
        else if (pressed & KEY_START) { screen = PAUSE; selected = 1; }
        else if ((pressed & KEY_R) && transcript_page + 1 < transcript_page_count()) transcript_page++;
        else if ((pressed & KEY_L) && transcript_page) transcript_page--;
        return;
    }
    if (screen == COMMAND) { keyboard_pressed(pressed); return; }
    if (screen == LETTERS || screen == WORDS) { speech_pressed(pressed); return; }
    if (screen == DIAL) { dial_pressed(pressed); return; }
    if (screen == FINISHED_MENU) {
        if (pressed & KEY_UP) rotate(&selected, sizeof(finished_options) / sizeof(finished_options[0]), false);
        else if (pressed & KEY_DOWN) rotate(&selected, sizeof(finished_options) / sizeof(finished_options[0]), true);
        else if (pressed & KEY_B) back_out();
        else if (pressed & KEY_A) {
            if (selected == 0) begin_story(false);
            else if (selected == 1) restore_saved_game();
            else {
                selected = chapter;
                screen = LAUNCHER;
            }
        }
        return;
    }
    if (screen == FINISHED) {
        if (pressed & KEY_R && page + 1 < page_count) page++;
        else if (pressed & KEY_L && page) page--;
        else if (pressed & (KEY_A | KEY_B)) {
            if (page + 1 < page_count) {
                if (pressed & KEY_A) page++;
                else page = page_count - 1;
            } else if (finished_prompt) {
                if (pressed & KEY_A) {
                    selected = 0;
                    screen = FINISHED_MENU;
                }
            } else if (pressed & KEY_A) screen = LAUNCHER;
        } else if ((pressed & KEY_SELECT) && !finished_prompt) {
            screen = LAUNCHER;
        }
        return;
    }
    if (pressed & KEY_B) { back_out(); return; }
    if (screen == TEXT_REVIEW) {
        if (pressed & (KEY_R | KEY_RIGHT | KEY_A) && page + 1 < page_count) page++;
        else if (pressed & (KEY_L | KEY_LEFT) && page) page--;
        return;
    }
    if (screen == PAUSE && (pressed & KEY_START)) { screen = EXPLORE; return; }
    unsigned count = list_count();
    if (pressed & KEY_UP) { rotate(&selected, count, false); return; }
    if (pressed & KEY_DOWN) { rotate(&selected, count, true); return; }
    if (count > MENU_LINES && (pressed & KEY_L)) {
        selected = selected >= MENU_LINES ? selected - MENU_LINES : 0;
        return;
    }
    if (count > MENU_LINES && (pressed & KEY_R)) {
        selected = selected + MENU_LINES < count ? selected + MENU_LINES : count - 1;
        return;
    }
    if (!(pressed & KEY_A) || !count) return;
    switch (screen) {
        case SLOT:
            if (selected == 2) screen = LAUNCHER;
            else if (selected == 1 || save_exists(SRAM, chapter)) begin_story(selected == 0);
            break;
        case BAG: pick_object(carried.id[selected]); break;
        case NEARBY: focus = selected; pick_object(nearby.id[selected]); break;
        case VERBS:
        case GROUP:
        case ORDERS_MENU: handle_verb(); break;
        case CATEGORIES:
            category_row = selected;
            choose_action_list(GROUP, groups[selected]);
            break;
        case SECOND_OBJECT: {
            char noun[Z_NAME_MAX];
            story_parser_noun(second.id[selected], noun);
            run_action(pending_action, noun);
            break;
        }
        case HEADING: run_action(pending_action, path_names[selected]); break;
        case ROOM_ACTIONS:
            if (room_options[selected].command) send_command(room_options[selected].command);
            else open_room_menu(room_options[selected].destination);
            break;
        case SPEAK:
            if (selected < sizeof(speech_titles) / sizeof(speech_titles[0])) play_speech(selected);
            else send_command(selected == 4 ? "yes" : "no");
            break;
        case PATHS: send_command(path_commands[path_options[selected]]); break;
        case PAUSE: handle_pause(); break;
        case PLACES:
            map_room = places[selected];
            map_cursor = MAP_CENTER;
            screen = MAP;
            break;
        case RETURN_CONFIRM:
            if (selected) screen = LAUNCHER;
            else back_out();
            break;
        default: break;
    }
}

static unsigned scene_animation_stage(void) {
    if (chapter == 2 && room_object == ZORK3_TREASURY && successful_ending) {
        const char *twins = find_substring(response, "two identical mages");
        const char *dissolve = find_substring(response, "your counterpart dissolves into a mist and disappears");
        unsigned first = page_offsets[page];
        unsigned end = page + 1 < page_count ? page_offsets[page + 1] : response_size;
        unsigned twin_at = twins ? (unsigned)(twins - response) : response_size;
        unsigned dissolve_at = dissolve ? (unsigned)(dissolve - response) : response_size;
        if (twins && twin_at >= first && twin_at < end) return 1;
        if (dissolve && dissolve_at < first) return 2;
        if (twins && twin_at < first && (!dissolve || dissolve_at >= end)) return 1;
        if (dissolve && dissolve_at < end) return 2;
        return 0;
    }
    if (chapter != 1 || room_object != 230) return 0;
    const char *cue = "flare and disappear";
    unsigned end = page + 1 < page_count ? page_offsets[page + 1] : response_size;
    for (unsigned i = 0; i < response_size; i++) {
        unsigned j = 0;
        while (cue[j] && i + j < response_size && response[i + j] == cue[j]) j++;
        if (!cue[j]) return i >= end ? 0 : i >= page_offsets[page] ? 1 : 2;
    }
    // A restored final snapshot has no narrated transition left to replay.
    return 2;
}

int main(void) {
    video_init();
    render();
    present();
    uint16_t previous = buttons();
    for (;;) {
        wait_frame();
        entropy_state++;
        uint16_t held_keys = buttons();
        uint16_t pressed = held_keys & ~previous;
        previous = held_keys;
        bool animated = !busy && (screen == EXPLORE || screen == FINISHED) &&
                        room_visuals_animate(chapter, room_object, scene_animation_stage());
        if (!busy && !pressed && !animated) continue;
        if (!busy) update(pressed);
        if (busy) complete_turn();
        render();
        present();
    }
}
