#include "interaction.h"
#include "room_map.h"
#include "room_visuals.h"
#include "save.h"
#include "transcript_codec.h"
#include "visual_state.h"

enum {
    ART_Y = 15, ART_HEIGHT = 76, TEXT_Y = 91, FOOT_Y = 145,
    STORY_LINES = 4, MENU_LINES = 4, LOG_SCREEN_LINES = 11,
    DPAD_REPEAT_DELAY = 20, DPAD_REPEAT_INTERVAL = 5,
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
#ifdef VISUAL_QA
static uint8_t qa_sram[SRAM_SIZE];
#define SRAM qa_sram
#else
#define SRAM ((volatile uint8_t *)0x0E000000)
#endif

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
    FINISHED_MENU, RESTORE_CONFIRM, LEARNED_SPELLS, HINTS, HINT_BROWSER, NEXT_STEPS
} Screen;

typedef struct { Screen screen; unsigned row; } MenuCursor;

static Screen screen, command_back;
static MenuCursor object_origin, room_menu_origin, category_origin, argument_origin, speech_origin;
static unsigned chapter, focus, selected, room_object;
static uint8_t path_options[12];
static unsigned path_count;
static bool verb_is_bag, busy, has_progress, dark;
static bool pause_has_save;
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
typedef struct {
    bool valid, carried;
    uint8_t chapter, count;
    uint16_t room, object;
    uint32_t generation;
    Action actions[MAX_ACTIONS];
} PrimaryMenuCache;
static PrimaryMenuCache primary_menu_cache[2];
static unsigned primary_menu_recent;
static uint32_t primary_menu_generation;
static bool menu_filter_running, menu_filter_armed;
static uint32_t menu_filter_ticks;
static struct {
    unsigned chapter, id;
    uint16_t room;
    bool carried;
} menu_filter_key;
static void invalidate_primary_menu_cache(void) {
    // A command can change any room/object/action interaction state; one
    // generation bump retires both entries without walking the array.
    primary_menu_generation++;
    if (story_menu_filter_active()) story_menu_filter_cancel();
    menu_filter_running = false;
    menu_filter_armed = false;
}
static PrimaryMenuCache *cached_primary_menu(void) {
    for (unsigned i = 0; i < 2; i++) {
        PrimaryMenuCache *entry = &primary_menu_cache[i];
        if (entry->valid && entry->generation == primary_menu_generation &&
            entry->chapter == chapter && entry->carried == verb_is_bag &&
            entry->room == z_global(0) && entry->object == action_object)
            return entry;
    }
    return NULL;
}
#ifdef PROFILE
typedef struct {
    uint16_t ticks, peak_ticks, probes, effects;
    uint8_t offers, cache_hit;
    uint16_t builds;
} MenuProfile;
static MenuProfile menu_profile;
#endif
static uint8_t places[256];
static char room_name[Z_NAME_MAX], command[COMMAND_SIZE];
static unsigned key_cursor, dial_digits, dial_cursor, category_row;
static uint8_t digits[3];
static WordMode word_mode;
static unsigned letter_cursor, word_cursor, word_count, spoken_count;
static uint16_t words[1024], spoken[WORDS_PER_PHRASE];
static void collect_paths(void);
static unsigned hint_browser_count(void);

static char response[7168];
static uint16_t page_offsets[256];
static unsigned response_size, page_count, page, page_before_review;
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
    {"Paths", PATHS, NULL}, {"Speak...", SPEAK, NULL}, {"Choose command words...", LETTERS, NULL},
    {"Nearby objects", NEARBY, NULL},
    {"Listen", EXPLORE, "listen"}, {"Wait", EXPLORE, "wait"}, {"Look", EXPLORE, "look"},
    {"Score", EXPLORE, "score"}, {"Take all", EXPLORE, "take all"}, {"Drop all", EXPLORE, "drop all"},
    {"Pray", EXPLORE, "pray"}, {"Launch", EXPLORE, "launch"}, {"Dive", EXPLORE, "dive"}
};
static const struct { const char *name; WordMode mode; const char *command; } speech_options[] = {
    {"Say...", WORDS_SAY, NULL}, {"Answer...", WORDS_ANSWER, NULL}, {"Greet...", WORDS_GREET, NULL},
    {"Yes", WORDS_SAY, "yes"}, {"No", WORDS_SAY, "no"}
};
static const char *const wizard_spell_names[] = {
    "fall", "float", "freeze", "fence", "fierce", "ferment", "fear",
    "feeble", "fumble", "filch", "fantasize", "fireproof"
};
static const char *const word_titles[] = {
    [WORDS_SAY] = "Say", [WORDS_ANSWER] = "Answer",
    [WORDS_COMMAND] = "Command words", [WORDS_GREET] = "Greet"
};
static const char *const category_options[] = {"Inspect", "Handle", "Use", "Combine", "Use a tool", "Movement"};
static const ActionGroup groups[] = {INSPECT, HANDLE, USE, COMBINE, TOOLS, MOVE};
static const char *const pause_options[] = {
    "Resume", "Story so far", "Save game", "Restore game", "Map", "Read last response",
    "Enter command...", "Controls", "Return to trilogy", "Hints"
};
enum { PAUSE_HINTS_ROW = sizeof(pause_options) / sizeof(pause_options[0]) - 1, HINT_STEPS = 3 };
typedef struct {
    uint8_t chapter, room;
    const char *steps[HINT_STEPS];
} RoomHint;
static const RoomHint room_hints[] = {
    {0, 78, {"The Cyclops pays attention to what you carry.",
             "Offer him lunch, then think about what he asks for.",
             "Give him lunch and then the open bottle of water. He will fall asleep and leave the stairs unguarded."}},
    {0, 129, {"The book on the altar is worth reading closely.",
              "Pay attention to where the original lines of the commandment begin.",
              "Read the first letters of the commandment's lines. They spell a name that frightens the Cyclops."}},
    {0, 205, {"Try a harmless command and notice what comes back to you.",
              "The sound in this room is part of the puzzle.",
              "Send ECHO from Choose command words, then take the platinum bar."}},
    {1, 238, {"The door expects a spoken answer.",
              "Think of a place where people draw up water.",
              "Use Speak, then Answer, and select WELL."}},
    {1, 119, {"Look at the bucket, and consider what you can put in it.",
              "The bucket follows the weight of the water.",
              "Enter the bucket and pour water into it. Recover the water with the teapot to come back down."}},
    {1, 159, {"The stone can be moved with help from magic.",
              "The white book explains how to direct the Wizard's wand.",
              "Aim or wave the wand at the menhir. Then use Say to cast FLOAT."}},
    {2, 181, {"Watch the water for someone who might hear you.",
              "When the ship comes in sight, try getting the sailor's attention.",
              "Use Hail sailor while the ship is visible. Take what the sailor throws you."}},
    {2, ZORK3_BEAM_ROOM, {"The light beam is part of a mechanism.",
                          "Leave something solid in the beam, then try the controls nearby.",
                          "Use the sword to interrupt the beam. Press the red button in the Button Room, then go north into the mirror."}},
    {2, ROYAL_PUZZLE_ROOM, {"Look to survey the square around you.",
                            "Sandstone walls can slide. Marble walls stay put.",
                            "At the starting square, push the east wall, then use Paths to move south."}}
};
typedef struct {
    const char *title;
    const char *const *clues;
    uint16_t parent, first, count;
} HintEntry;
typedef struct {
    uint8_t chapter, room;
    uint16_t target, entry;
    const char *label;
} ContextHint;
#include "invisiclues.inc"

enum { HINT_TEXT_LINES = 7 };
static unsigned hint_step, hint_offset, hint_menu;
static const HintEntry *hint_topic;
static const char *hint_context_label;
static const char *const slot_options[] = {"Continue saved game", "New game", "Back"};
static const char *const return_options[] = {"Keep exploring", "Return to trilogy"};
static const char *const restore_options[] = {"Keep playing", "Restore saved game"};
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

static void direction_hint(int x, uint16_t direction, const char *action) {
    dpad_badge(x, 148, 11, direction, PANEL, LINE, GOLD);
    label(x + 15, 150, action, PAPER);
}

static void vertical_hint(int x, const char *action) {
    direction_hint(x, KEY_UP, "");
    direction_hint(x + 13, KEY_DOWN, action);
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
    invalidate_primary_menu_cache();
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
    invalidate_primary_menu_cache();
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
    invalidate_primary_menu_cache();
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

static void learn_wizard_spells(void) {
    static const char *const introductions[] = {
        "The Wizard, in a deep and resonant voice, speaks the word \"",
        "The Wizard incants \"",
        "The Wizard snatches the wand away, muttering the word \""
    };
    uint16_t learned = visual_state_wizard_spells();
    for (unsigned i = 0; i < sizeof(introductions) / sizeof(introductions[0]); i++) {
        const char *word = find_substring(response, introductions[i]);
        if (!word) continue;
        while (*word != '"') word++;
        word++;
        char name[12];
        unsigned length = 0;
        while (word[length] && word[length] != '!' && word[length] != '"' && length + 1 < sizeof(name)) {
            name[length] = word[length];
            length++;
        }
        if (word[length] != '!' && word[length] != '"') continue;
        name[length] = 0;
        for (unsigned spell = 0; spell < sizeof(wizard_spell_names) / sizeof(wizard_spell_names[0]); spell++)
            if (text_equal(name, wizard_spell_names[spell])) learned |= 1u << spell;
    }
    visual_state_restore_wizard_spells(learned);
}

static void complete_turn(void) {
    ZResult result = z_run(6000);
    if (result == Z_BUSY) return;
    busy = false;
    if (chapter == 0 && text_has(response, "Commandment #12592"))
        visual_state_mark_cyclops_clue_read();
    if (chapter == 1) learn_wizard_spells();
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
        if (chapter == 2 && room_object == 143 &&
            text_has(response, "With the help of the ladder, you exit the puzzle."))
            draw_puzzle_exit_ladder();
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
        int text_right = 232;
        fill(0, TEXT_Y + 1, SCREEN_W, 11, INK);
        draw_target_focus_box(true);
        if (focused) draw_story_icon(chapter, focused, 8, TEXT_Y + 2);
        if (!busy && page_count > 1) {
            char counter[8], total[4];
            number(counter, page + 1);
            number(total, page_count);
            append_text(counter, sizeof(counter), "/");
            append_text(counter, sizeof(counter), total);
            int arrow_width = 9;
            int next_x = text_right - arrow_width;
            int counter_x = next_x - 4 - label_width(counter);
            int previous_x = counter_x - 4 - arrow_width;
            Color previous_color = page ? GOLD : DIM;
            Color next_color = page + 1 < page_count ? GOLD : DIM;
            dpad_badge(previous_x, TEXT_Y + 2, arrow_width, KEY_LEFT, PANEL, LINE, previous_color);
            label(counter_x, TEXT_Y + 3, counter, PAPER);
            dpad_badge(next_x, TEXT_Y + 2, arrow_width, KEY_RIGHT, PANEL, LINE, next_color);
            text_right = previous_x - 6;
        }
        paragraph(text_x, TEXT_Y + 3, text_right - text_x, 1, item, GOLD);
    }
    int story_y = reviewing || barrow_ending || screen == FINISHED ? TEXT_Y + 5 : TEXT_Y + 15;
    if (busy) label(8, story_y, "One moment...", DIM);
    else paragraph(8, story_y, 224, STORY_LINES, response + page_offsets[page], PAPER);
    footer();
    if (reviewing) {
        key_hint(6, "L/R", "Page");
        key_hint(90, "B", "Back");
    } else if (screen == FINISHED) {
        if (page + 1 < page_count) {
            key_hint(6, "A", "Read more");
            key_hint(88, "B", "Skip to end");
        } else if (finished_prompt) {
            key_hint(6, "A", "Options");
            key_hint(83, "L/R", "Page");
        } else {
            key_hint(6, "A", "Trilogy menu");
            key_hint(106, "L/R", "Page");
        }
    } else {
        direction_hint(6, KEY_UP, "Paths");
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
    vertical_hint(6, "Choose");
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
    vertical_hint(6, "Choose");
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

static const char *learned_spell_at(unsigned row) {
    uint16_t learned = visual_state_wizard_spells();
    for (unsigned i = 0; i < sizeof(wizard_spell_names) / sizeof(wizard_spell_names[0]); i++) {
        if (!(learned & (1u << i))) continue;
        if (!row) return wizard_spell_names[i];
        row--;
    }
    return NULL;
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
        case SPEAK: return sizeof(speech_options) / sizeof(speech_options[0]) +
                           (chapter == 1 && learned_spell_at(0) != NULL);
        case LEARNED_SPELLS: {
            unsigned count = 0;
            while (learned_spell_at(count)) count++;
            return count;
        }
        case ROOM_ACTIONS: return sizeof(room_options) / sizeof(room_options[0]);
        case PATHS: return path_count;
        case PAUSE: return sizeof(pause_options) / sizeof(pause_options[0]);
        case HINT_BROWSER: return hint_browser_count();
        case PLACES: return place_count;
        case RETURN_CONFIRM: return 2;
        case RESTORE_CONFIRM: return 2;
        default: return 0;
    }
}

#ifdef PROFILE
static void draw_menu_profile(void) {
    char text[48] = "menu ", n[8];
    number(n, menu_profile.probes);
    append_text(text, sizeof(text), n);
    append_text(text, sizeof(text), "p ");
    number(n, menu_profile.effects);
    append_text(text, sizeof(text), n);
    append_text(text, sizeof(text), "e ");
    unsigned ms = ((unsigned)menu_profile.ticks * 1000u + 8192u) / 16384u;
    number(n, (int)ms);
    append_text(text, sizeof(text), n);
    append_text(text, sizeof(text), "ms pk ");
    ms = ((unsigned)menu_profile.peak_ticks * 1000u + 8192u) / 16384u;
    number(n, (int)ms);
    append_text(text, sizeof(text), n);
    append_text(text, sizeof(text), "ms ");
    number(n, menu_profile.offers);
    append_text(text, sizeof(text), n);
    append_text(text, sizeof(text), "a");
    if (menu_profile.cache_hit) append_text(text, sizeof(text), " cache");
    fill(0, ART_Y, 190, 9, INK);
    label(3, ART_Y + 1, text, GOLD);
}
#endif

static void draw_drawer(void) {
    char object[Z_NAME_MAX], item[Z_NAME_MAX];
    const char *title_text = "";
    unsigned count = list_count(), first = view_start(count);
    unsigned back_direction = screen == PATHS ? direction_to_previous_room() : TRAIL_DIRECTIONS;
    switch (screen) {
        case BAG: title_text = "Carried items"; break;
        case NEARBY: title_text = "Nearby objects"; break;
        case VERBS:
            story_name(chapter, action_object, object);
            if (menu_filter_running) append_text(object, sizeof(object), " ...");
            title_text = object;
            break;
        case GROUP: story_name(chapter, action_object, object); title_text = object; break;
        case CATEGORIES: title_text = "More actions"; break;
        case ORDERS_MENU: title_text = "Give an order"; break;
        case SECOND_OBJECT: title_text = pending_action.name; break;
        case HEADING: title_text = "Choose a direction"; break;
        case SPEAK: title_text = "Speak"; break;
        case LEARNED_SPELLS: title_text = "Learned spells"; break;
        case ROOM_ACTIONS: title_text = "Room actions"; break;
        case PATHS: title_text = "Paths"; break;
        case PAUSE: title_text = "Paused"; break;
        case PLACES: title_text = "Places visited"; break;
        case RETURN_CONFIRM: title_text = "Leave this game?"; break;
        case RESTORE_CONFIRM: title_text = "Lose unsaved progress?"; break;
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
            case SPEAK: name = i < sizeof(speech_options) / sizeof(speech_options[0])
                               ? speech_options[i].name : "Learned spells..."; break;
            case LEARNED_SPELLS: name = learned_spell_at(i); break;
            case ROOM_ACTIONS: name = room_options[i].name; break;
            case PATHS: name = path_names[path_options[i]]; break;
            case PAUSE:
                name = i == 3 && !pause_has_save ? "Restore game (no save)" : pause_options[i];
                break;
            case PLACES: z_name(places[i], item, sizeof(item)); break;
            case RETURN_CONFIRM: name = return_options[i]; break;
            case RESTORE_CONFIRM: name = restore_options[i]; break;
            default: name = "";
        }
        if (id) story_name(chapter, id, item);
        drawer_row(first, i, name, screen != PAUSE || i != 3 || pause_has_save, id);
        if (screen == PATHS) {
            unsigned direction = path_options[i];
            const uint8_t *exit;
            int y = TEXT_Y + 15 + (i - first) * 10;
            if (direction == back_direction) label(110, y, "Way back", BACK);
            if (z_property(room_object, path_properties[direction], &exit) == 2)
                label(165, y, "Conditional", DIM);
        }
    }
#ifdef PROFILE
    if (screen == VERBS || screen == GROUP) draw_menu_profile();
#endif
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
        vertical_hint(6, "Chapter");
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
        "Left / Right: Page through text.\n"
        "Right triangles show diagonals.\n"
        "Teal marks a known way back.\n"
        "Dim routes may need a condition.\n"
        "L / R: Focus on object or room.\n"
        "A: Act or confirm.\n"
        "B: Look or go back.\n"
        "Select: Open your bag.\n"
        "Start: Pause menu has Story so far.\n\n"
        "Room actions: Nearby objects,\n"
        "speech, and other commands.", PAPER);
    footer();
    key_hint(6, "B", "Back");
}

static const RoomHint *current_room_hint(void) {
    if (dark) return NULL;
    for (unsigned i = 0; i < sizeof(room_hints) / sizeof(room_hints[0]); i++)
        if (room_hints[i].chapter == chapter && room_hints[i].room == room_object) return &room_hints[i];
    return NULL;
}

static unsigned room_hint_rows(void) {
    return hint_menu == 0 && current_room_hint() != NULL;
}

static bool hint_target_nearby(unsigned target) {
    if (!target) return true;
    for (unsigned i = 0; i < nearby.count; i++)
        if (nearby.id[i] == target) return true;
    return false;
}

static const ContextHint *context_hint_at(unsigned row) {
    if (hint_menu || dark) return NULL;
    for (unsigned i = 0; i < sizeof(contextual_hints) / sizeof(contextual_hints[0]); i++) {
        const ContextHint *hint = &contextual_hints[i];
        if (hint->chapter != chapter || hint->room != room_object || !hint_target_nearby(hint->target)) continue;
        if (!row) return hint;
        row--;
    }
    return NULL;
}

static unsigned local_hint_rows(void) {
    unsigned contextual = 0;
    while (context_hint_at(contextual)) contextual++;
    return room_hint_rows() + contextual;
}

static unsigned hint_guide_offset(void) {
    return local_hint_rows() + (hint_menu == 0);
}

static bool is_next_steps_row(unsigned row) {
    return hint_menu == 0 && row == local_hint_rows();
}

static unsigned hint_browser_count(void) {
    return invisiclues[chapter][hint_menu].count + hint_guide_offset();
}

static const HintEntry *hint_browser_entry(unsigned row) {
    const HintEntry *guide = invisiclues[chapter];
    unsigned room_rows = room_hint_rows();
    if (row < room_rows) return NULL;
    if (is_next_steps_row(row)) return NULL;
    const ContextHint *context = context_hint_at(row - room_rows);
    if (context) return &guide[context->entry];
    return &guide[guide[hint_menu].first + row - hint_guide_offset()];
}

static const ContextHint *hint_browser_context(unsigned row) {
    unsigned room_rows = room_hint_rows();
    return row < room_rows ? NULL : context_hint_at(row - room_rows);
}

static void draw_hint_browser(void) {
    unsigned count = hint_browser_count();
    enum { HINT_MENU_ROWS = 3 };
    unsigned first = selected / HINT_MENU_ROWS * HINT_MENU_ROWS;
    char position[16], total[8];
    number(position, selected + 1);
    number(total, count);
    append_text(position, sizeof(position), "/");
    append_text(position, sizeof(position), total);
    header(invisiclues[chapter][hint_menu].title, position);
    fill(0, ART_Y, SCREEN_W, FOOT_Y - ART_Y, PANEL);
    for (unsigned i = first; i < count && i < first + HINT_MENU_ROWS; i++) {
        const HintEntry *entry = hint_browser_entry(i);
        const ContextHint *context = hint_browser_context(i);
        int y = 19 + (i - first) * 40;
        if (i == selected) {
            fill(6, y, 228, 36, INK);
            fill(7, y + 4, 2, 28, GOLD);
        }
        const char *title = context ? context->label : entry ? entry->title :
                            is_next_steps_row(i) ? "Next steps" : "Hints for this room";
        paragraph(14, y + 3, 210, 3, title,
                  i == selected ? GOLD : PAPER);
    }
    footer();
    vertical_hint(6, "Choose");
    key_hint(83, "A", "Open");
    key_hint(185, "B", "Back");
}

static unsigned hint_count(void) {
    return hint_topic ? hint_topic->count : HINT_STEPS;
}

enum { NEXT_STEP_LIMIT = 4 };

static unsigned story_next_steps(const char *steps[NEXT_STEP_LIMIT]) {
    unsigned count = 0;
    unsigned player = chapters[chapter].player;
    if (chapter == 0) {
        if (room_seen(129) && !visual_state_cyclops_clue_read()) {
            unsigned book_location = z_parent(196);
            if (book_location == player) steps[count++] = "Read the black book in your bag.";
            else if (book_location == 129 || (book_location == 35 && z_parent(35) == 129))
                steps[count++] = "Read the black book at the Altar.";
        }
        if (room_seen(216) && z_parent(96) == player && !z_attr(29, ATTR_OPEN))
            steps[count++] = "Revisit the Grating Room with the skeleton key.";
        if (room_seen(111) && z_parent(208) == 111 && z_parent(player) != 208)
            steps[count++] = "Board the boat at Dam Base.";
    } else if (chapter == 1) {
        if (room_seen(238) && !z_attr(7, ATTR_OPEN))
            steps[count++] = "Try an answer to the riddle in the Riddle Room.";
        if (room_seen(ZORK2_AQUARIUM_ROOM)) {
            if (z_parent(ZORK2_SERPENT) == ZORK2_AQUARIUM && z_parent(223) == player)
                steps[count++] = "Bring the sword to the Aquarium Room.";
            else if (z_parent(31) == ZORK2_AQUARIUM_ROOM)
                steps[count++] = "Collect the exposed sphere in the Aquarium Room.";
        }
        if (room_seen(119) && z_parent(52) == 119 && z_parent(186) == 107 &&
            z_parent(107) == player)
            steps[count++] = "Return to the Circular Room with the full teapot.";
    } else {
        if (!room_seen(160) && room_seen(ZORK3_BEAM_ROOM) && z_parent(202) == player)
            steps[count++] = "Bring the sword to the Beam Room.";
        if (!room_seen(160) && room_seen(ZORK3_BUTTON_ROOM) && z_parent(202) == ZORK3_BEAM_ROOM &&
            !zork3_mirror_panel_open())
            steps[count++] = "Check the controls in the Button Room.";
        if (room_seen(ZORK3_PRISON_CELL_ALT) && z_parent(13) == player && !z_attr(209, ATTR_OPEN))
            steps[count++] = "Revisit the bronze door in the Prison Cell with the strange key.";
    }
    return count;
}

static void draw_next_steps(void) {
    header("Next steps", NULL);
    fill(0, ART_Y, SCREEN_W, FOOT_Y - ART_Y, PANEL);
    const char *steps[NEXT_STEP_LIMIT];
    unsigned count = story_next_steps(steps);
    if (!count) paragraph(8, 28, 224, 3, "No suggestions yet. Explore and check back later.", DIM);
    for (unsigned i = 0; i < count; i++) {
        int y = 21 + i * 30;
        fill(8, y + 4, 2, 7, GOLD);
        paragraph(16, y, 216, 2, steps[i], PAPER);
    }
    footer();
    key_hint(185, "B", "Back");
}

static const char *hint_text(void) {
    if (!hint_step) return "Each hint gives away a little more.";
    return hint_topic ? hint_topic->clues[hint_step - 1] : current_room_hint()->steps[hint_step - 1];
}

static unsigned next_hint_page(const char *text, unsigned offset) {
    const char *line = text + offset;
    for (unsigned i = 0; i < HINT_TEXT_LINES && *line; i++) {
        const char *end;
        line = wrap_line(line, 224, &end);
    }
    return *line ? (unsigned)(line - text) : offset;
}

static void draw_hints(void) {
    char progress[16], count[8];
    number(progress, hint_step);
    number(count, hint_count());
    append_text(progress, sizeof(progress), "/");
    append_text(progress, sizeof(progress), count);
    header("Hints", hint_step ? progress : NULL);
    fill(0, ART_Y, SCREEN_W, FOOT_Y - ART_Y, PANEL);
    const char *question = hint_context_label ? hint_context_label : hint_topic ? hint_topic->title : room_name;
    paragraph(8, 22, 224, 4, question, GOLD);
    const char *message = hint_text();
    paragraph(8, 68, 224, HINT_TEXT_LINES, message + hint_offset, PAPER);
    footer();
    if (hint_step < hint_count()) key_hint(6, "A", hint_step ? "Next hint" : "Reveal hint");
    if (hint_offset || next_hint_page(message, hint_offset) != hint_offset) vertical_hint(82, "Text");
    if (hint_step > 1) key_hint(127, "<", "Previous");
    key_hint(185, "B", "Back");
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
    header(word_titles[word_mode], NULL);
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
    if (year) key_hint(6, "PAD", "Adjust");
    else vertical_hint(6, "Adjust");
    key_hint(83, "A", "Confirm");
    key_hint(159, "B", "Back");
}

static void render(void) {
    if (screen == LAUNCHER || screen == SLOT) draw_launcher();
    else if (screen == COMMAND) draw_keyboard();
    else if (screen == CONTROLS) draw_controls();
    else if (screen == HINTS) draw_hints();
    else if (screen == HINT_BROWSER) draw_hint_browser();
    else if (screen == NEXT_STEPS) draw_next_steps();
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

static bool action_equal(Action a, Action b) {
    if (a.kind != b.kind) return false;
    if ((!a.name) != (!b.name) || (a.name && !text_equal(a.name, b.name))) return false;
    if ((!a.verb) != (!b.verb) || (a.verb && !text_equal(a.verb, b.verb))) return false;
    if ((!a.argument) != (!b.argument) || (a.argument && !text_equal(a.argument, b.argument))) return false;
    return true;
}

static void refresh_filter_menu(void) {
    Action previous = selected < action_count ? action_list[selected] : (Action){0};
    action_count = story_menu_filter_display(action_list, MAX_ACTIONS);
    unsigned found = action_count;
    if (action_count) {
        for (unsigned i = 0; i < action_count; i++) {
            if (action_equal(previous, action_list[i])) { found = i; break; }
        }
        if (found == action_count)
            found = selected < action_count ? selected : action_count - 1;
    } else found = 0;
    selected = found;
}

static void choose_action_list(Screen next, ActionGroup group) {
    open_menu(next);
    if (group == PRIMARY) {
        PrimaryMenuCache *cached = cached_primary_menu();
        if (cached) {
            action_count = cached->count;
            for (unsigned i = 0; i < action_count; i++) action_list[i] = cached->actions[i];
            menu_filter_running = false;
            menu_filter_armed = false;
#ifdef PROFILE
            menu_profile.ticks = 0;
            menu_profile.peak_ticks = 0;
            menu_profile.probes = 0;
            menu_profile.effects = 0;
            menu_profile.offers = (uint8_t)action_count;
            menu_profile.cache_hit = 1;
            menu_profile.builds++;
#endif
            return;
        }
        // Returning from a category while the same object is still being
        // filtered: keep the probe batch and show its current rows.
        if (story_menu_filter_active() && menu_filter_key.chapter == chapter &&
            menu_filter_key.id == action_object && menu_filter_key.carried == verb_is_bag &&
            menu_filter_key.room == z_global(0)) {
            action_count = story_menu_filter_display(action_list, MAX_ACTIONS);
            menu_filter_running = true;
            menu_filter_armed = false;
            return;
        }
        // Show state-compatible candidates immediately; the main loop advances
        // the remaining filter one candidate (or effect probe) per frame.
        action_count = story_actions(chapter, action_object, verb_is_bag, group, action_list);
        story_menu_filter_begin(chapter, action_object, verb_is_bag, action_list, action_count);
        action_count = story_menu_filter_display(action_list, MAX_ACTIONS);
        menu_filter_key.chapter = chapter;
        menu_filter_key.id = action_object;
        menu_filter_key.carried = verb_is_bag;
        menu_filter_key.room = (uint16_t)z_global(0);
        menu_filter_running = true;
        menu_filter_armed = false;
        menu_filter_ticks = 0;
#ifdef PROFILE
        menu_profile.cache_hit = 0;
        menu_profile.ticks = 0;
        menu_profile.peak_ticks = 0;
        menu_profile.probes = 0;
        menu_profile.effects = 0;
        menu_profile.offers = (uint8_t)action_count;
#endif
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

static void open_word_picker(WordMode mode) {
    word_mode = mode;
    speech_origin = (MenuCursor){screen, selected};
    letter_cursor = spoken_count = 0;
    screen = LETTERS;
}

static void collect_words(void) {
    word_cursor = word_count = 0;
    unsigned size = z_dictionary_size();
    for (unsigned i = 0; i < size && word_count < sizeof(words) / sizeof(words[0]); i++) {
        char word[Z_WORD_MAX];
        speech_word(i, word);
        if (word[0] != 'a' + (int)letter_cursor) continue;
        if (chapter == 0 && !visual_state_cyclops_clue_read() &&
            (text_equal(word, "odysseus") || text_equal(word, "ulysses"))) continue;
        words[word_count++] = i;
    }
    screen = WORDS;
}

static void speech_pressed(uint16_t pressed) {
    if (pressed & KEY_START) {
        char phrase[WORDS_PER_PHRASE * Z_WORD_MAX], line[COMMAND_SIZE];
        phrase_text(phrase);
        if (speech_command(word_mode, phrase, line)) send_command(line);
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
            if (pause_has_save) {
                if (has_progress) restore_saved_game();
                else open_menu(RESTORE_CONFIRM);
            }
            break;
        case 4: open_map(); break;
        case 5: page_before_review = page; page = 0; screen = TEXT_REVIEW; break;
        case 6: open_command(PAUSE); break;
        case 7: screen = CONTROLS; break;
        case 8:
            if (has_progress) screen = LAUNCHER;
            else { screen = RETURN_CONFIRM; selected = 0; }
            break;
        case PAUSE_HINTS_ROW:
            hint_menu = 0;
            open_menu(HINT_BROWSER);
            break;
    }
}

static void explore_pressed(uint16_t pressed) {
    if (pressed & KEY_START) {
        pause_has_save = save_exists(SRAM, chapter);
        screen = PAUSE;
        selected = 0;
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
    else if ((pressed & KEY_LEFT) && page) page--;
    else if ((pressed & KEY_RIGHT) && page + 1 < page_count) page++;
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

static void clamp_action_row(void) {
    if (screen != VERBS && screen != GROUP && screen != ORDERS_MENU) return;
    if (!action_count) selected = 0;
    else if (selected >= action_count) selected = action_count - 1;
}

static void back_out(void) {
    switch (screen) {
        case SLOT: screen = LAUNCHER; break;
        case VERBS:
            if (story_menu_filter_active()) {
                story_menu_filter_cancel();
                menu_filter_running = false;
                menu_filter_armed = false;
            }
            return_to(object_origin);
            break;
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
        case LEARNED_SPELLS:
            screen = SPEAK;
            selected = sizeof(speech_options) / sizeof(speech_options[0]);
            break;
        case PLACES: screen = MAP; break;
        case STORY_LOG: screen = PAUSE; selected = 1; break;
        case CONTROLS: screen = PAUSE; selected = 7; break;
        case HINTS:
        case NEXT_STEPS: screen = HINT_BROWSER; break;
        case HINT_BROWSER:
            if (hint_menu) {
                const HintEntry *guide = invisiclues[chapter];
                unsigned child = hint_menu;
                hint_menu = guide[child].parent;
                selected = child - guide[hint_menu].first + hint_guide_offset();
            } else {
                screen = PAUSE;
                selected = PAUSE_HINTS_ROW;
            }
            break;
        case TEXT_REVIEW: page = page_before_review; screen = PAUSE; selected = 5; break;
        case RETURN_CONFIRM: screen = PAUSE; selected = 8; break;
        case RESTORE_CONFIRM: screen = PAUSE; selected = 3; break;
        case FINISHED_MENU:
            screen = FINISHED;
            page = page_count ? page_count - 1 : 0;
            break;
        default:
            if (story_menu_filter_active()) {
                story_menu_filter_cancel();
                menu_filter_running = false;
                menu_filter_armed = false;
            }
            screen = EXPLORE;
            break;
    }
    clamp_action_row();
}

static void update(uint16_t pressed) {
    clamp_action_row();
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
    if (screen == NEXT_STEPS) {
        if (pressed & KEY_B) back_out();
        return;
    }
    if (screen == HINTS) {
        if (pressed & KEY_B) back_out();
        else if ((pressed & KEY_A) && hint_step < hint_count()) {
            hint_step++;
            hint_offset = 0;
        } else if ((pressed & KEY_LEFT) && hint_step > 1) {
            hint_step--;
            hint_offset = 0;
        } else if (pressed & KEY_DOWN) {
            hint_offset = next_hint_page(hint_text(), hint_offset);
        } else if ((pressed & KEY_UP) && hint_offset) {
            const char *text = hint_text();
            unsigned previous = 0;
            for (unsigned next = next_hint_page(text, previous); next > previous && next < hint_offset;
                 next = next_hint_page(text, previous)) previous = next;
            hint_offset = previous;
        }
        return;
    }
    if (screen == HINT_BROWSER) {
        if (pressed & KEY_B) back_out();
        else if (pressed & KEY_UP) rotate(&selected, hint_browser_count(), false);
        else if (pressed & KEY_DOWN) rotate(&selected, hint_browser_count(), true);
        else if (pressed & KEY_A) {
            if (is_next_steps_row(selected)) {
                screen = NEXT_STEPS;
                return;
            }
            const HintEntry *entry = hint_browser_entry(selected);
            if (entry && !entry->clues) {
                hint_menu = (unsigned)(entry - invisiclues[chapter]);
                selected = 0;
            } else {
                hint_topic = entry;
                const ContextHint *context = hint_browser_context(selected);
                hint_context_label = context ? context->label : NULL;
                hint_step = hint_offset = 0;
                screen = HINTS;
            }
        }
        return;
    }
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
            else if (room_options[selected].destination == LETTERS) open_word_picker(WORDS_COMMAND);
            else open_room_menu(room_options[selected].destination);
            break;
        case SPEAK:
            if (selected == sizeof(speech_options) / sizeof(speech_options[0])) open_menu(LEARNED_SPELLS);
            else if (speech_options[selected].command) send_command(speech_options[selected].command);
            else open_word_picker(speech_options[selected].mode);
            break;
        case LEARNED_SPELLS: {
            char line[COMMAND_SIZE];
            const char *spell = learned_spell_at(selected);
            if (spell && speech_command(WORDS_SAY, spell, line)) send_command(line);
            break;
        }
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
        case RESTORE_CONFIRM:
            if (selected) restore_saved_game();
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

static void remember_primary_menu(void) {
    Action filtered[MAX_ACTIONS];
    unsigned count = story_menu_filter_display(filtered, MAX_ACTIONS);
    unsigned slot = primary_menu_recent ^ 1u;
    for (unsigned i = 0; i < 2; i++) {
        PrimaryMenuCache *entry = &primary_menu_cache[i];
        if (!entry->valid || entry->generation != primary_menu_generation) {
            slot = i;
            break;
        }
    }
    PrimaryMenuCache *entry = &primary_menu_cache[slot];
    entry->valid = true;
    entry->generation = primary_menu_generation;
    entry->chapter = (uint8_t)chapter;
    entry->carried = verb_is_bag;
    entry->room = (uint16_t)z_global(0);
    entry->object = (uint16_t)action_object;
    entry->count = (uint8_t)count;
    for (unsigned i = 0; i < count; i++) entry->actions[i] = filtered[i];
    primary_menu_recent = slot;
}

static uint16_t repeat_dpad(uint16_t held) {
    static uint16_t direction;
    static Screen origin;
    static unsigned countdown;
    uint16_t next = held & (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT);
    if (next != direction) {
        direction = next;
        origin = screen;
        countdown = DPAD_REPEAT_DELAY;
        return 0;
    }
    if (screen != origin) countdown = 0;
    if (!countdown || !direction || (direction & (direction - 1))) return 0;
    bool paging = screen == EXPLORE && page_count > 1 && (direction & (KEY_LEFT | KEY_RIGHT));
    if (!paging && !list_count() && screen != LAUNCHER && screen != FINISHED_MENU &&
        screen != MAP && screen != COMMAND && screen != LETTERS &&
        screen != WORDS && screen != DIAL) return 0;
    if (--countdown) return 0;
    countdown = DPAD_REPEAT_INTERVAL;
    return direction;
}

static void step_primary_menu_filter(void) {
    if (menu_filter_running && story_menu_filter_active()) {
        if (!menu_filter_armed) menu_filter_armed = true;
        else {
#ifdef PROFILE
            video_profile_timer_start();
#endif
            bool done = story_menu_filter_step();
#ifdef PROFILE
            uint16_t step_ticks = video_profile_timer_read();
            menu_filter_ticks += step_ticks;
            if (step_ticks > menu_profile.peak_ticks) menu_profile.peak_ticks = step_ticks;
            menu_profile.ticks = menu_filter_ticks > 0xFFFFu ? 0xFFFFu : (uint16_t)menu_filter_ticks;
            menu_profile.probes = (uint16_t)story_menu_probes_used();
            menu_profile.effects = (uint16_t)story_menu_effect_probes_used();
#endif
            if (screen == VERBS) refresh_filter_menu();
            if (done) {
                menu_filter_running = false;
                menu_filter_armed = false;
                remember_primary_menu();
                if (screen == VERBS)
                    selected = selected < action_count ? selected :
                               (action_count ? action_count - 1 : 0);
#ifdef PROFILE
                menu_profile.offers = primary_menu_cache[primary_menu_recent].count;
                menu_profile.cache_hit = 0;
                menu_profile.builds++;
#endif
            }
        }
    }
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
        uint16_t repeated = repeat_dpad(held_keys);
        if (!pressed) pressed = repeated;
        bool animated = !busy && (screen == EXPLORE || screen == FINISHED) &&
                        room_visuals_animate(chapter, room_object, scene_animation_stage());
        bool filtering = story_menu_filter_active();
        if (!busy && !pressed && !animated && !filtering) continue;
        if (!busy) update(pressed);
        if (busy) complete_turn();
        step_primary_menu_filter();
        render();
        present();
    }
}
