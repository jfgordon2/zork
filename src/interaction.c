#include "interaction.h"

#define OBJECT(label, verb) {label, verb, NULL, DO_OBJECT}
#define TARGET(label, verb, prep) {label, verb, prep, PICK_TARGET}
#define SELF(label, verb, prep) {label, verb, prep, PICK_SELF}
#define OFFER(label, verb, prep) {label, verb, prep, PICK_ITEM}
#define SENTENCE(label, verb) {label, verb, NULL, DO_LITERAL}
#define DIRECTION(label, prefix) {label, prefix, NULL, PICK_DIRECTION}
#define MENU(label, kind) {label, NULL, NULL, kind}
#define NUMBER(label, verb, kind) {label, verb, NULL, kind}

enum { ACTION_REPLY_SIZE = 192, MAX_NOUN_WORDS = 8 };

static bool action_needs_object_noun(Action action) {
    return action.kind == DO_OBJECT || action.kind == PICK_ITEM ||
           ((action.kind == PICK_TARGET || action.kind == PICK_SELF) && action.argument) ||
           (action.kind == PICK_DIRECTION && text_equal(action.verb, "push"));
}

static bool raw_action_command(Action action, const char *noun, const char *value,
                               char line[COMMAND_SIZE]) {
    line[0] = 0;
    if (!action.verb || !append_text(line, COMMAND_SIZE, action.verb)) return false;
    if (action.kind == PICK_ITEM) {
        return noun && *noun && value && *value && action.argument &&
               append_text(line, COMMAND_SIZE, " ") && append_text(line, COMMAND_SIZE, value) &&
               append_text(line, COMMAND_SIZE, " ") && append_text(line, COMMAND_SIZE, action.argument) &&
               append_text(line, COMMAND_SIZE, " ") && append_text(line, COMMAND_SIZE, noun);
    }
    if (action_needs_object_noun(action)) {
        if (!noun || !*noun || !append_text(line, COMMAND_SIZE, " ") ||
            !append_text(line, COMMAND_SIZE, noun)) return false;
    }
    if (action.argument &&
        (!append_text(line, COMMAND_SIZE, " ") || !append_text(line, COMMAND_SIZE, action.argument))) return false;
    if (value && (!append_text(line, COMMAND_SIZE, " ") || !append_text(line, COMMAND_SIZE, value))) return false;
    return true;
}

static bool parser_rejected(const char *reply) {
    return text_has(reply, "I don't know the word") ||
           text_has(reply, "I don't understand that sentence") ||
           text_has(reply, "That sentence isn't one I recognize") ||
           text_has(reply, "That sentence is not one I recognize");
}

static bool probe_action_form(Action action, const char *value,
                              const char *noun, char reply[ACTION_REPLY_SIZE]) {
    char line[COMMAND_SIZE];
    reply[0] = 0;
    if (!raw_action_command(action, noun, value, line)) return false;
    if (!z_probe(line, reply, ACTION_REPLY_SIZE, 20000)) return true;
    return !parser_rejected(reply);
}

static void copy_noun_words(const char *noun, unsigned first, unsigned count,
                            const unsigned starts[MAX_NOUN_WORDS],
                            const unsigned lengths[MAX_NOUN_WORDS], char out[Z_NAME_MAX]) {
    out[0] = 0;
    for (unsigned i = 0; i < count; i++) {
        if (i) append_text(out, Z_NAME_MAX, " ");
        unsigned at = 0;
        while (at < starts[first + i] && noun[at]) at++;
        unsigned end = starts[first + i] + lengths[first + i];
        while (at < end && noun[at] && at + 1 < Z_NAME_MAX) {
            unsigned used = 0;
            while (out[used]) used++;
            out[used] = noun[at++];
            out[used + 1] = 0;
        }
    }
}

// Some Z-machine grammars accept an object's short headword in an action but
// reject its full display name (for example, "climb mountain range" versus
// "climb mountain"). Probe the actual story parser and try shorter contiguous
// noun phrases, longest first, before exposing a generated default action.
static bool resolve_action_noun(Action action, unsigned object, const char *value,
                                char chosen[Z_NAME_MAX], char reply[ACTION_REPLY_SIZE]) {
    char full[Z_NAME_MAX];
    chosen[0] = 0;
    story_parser_noun(object, full);
    if (!*full) return false;
    if (probe_action_form(action, value, full, reply)) {
        append_text(chosen, Z_NAME_MAX, full);
        return true;
    }

    unsigned starts[MAX_NOUN_WORDS], lengths[MAX_NOUN_WORDS], words = 0;
    for (unsigned i = 0; full[i] && words < MAX_NOUN_WORDS;) {
        while (full[i] == ' ') i++;
        if (!full[i]) break;
        starts[words] = i;
        while (full[i] && full[i] != ' ') i++;
        lengths[words] = i - starts[words];
        words++;
    }
    for (unsigned count = words; count > 0; count--) {
        for (unsigned first = 0; first + count <= words; first++) {
            if (first == 0 && count == words) continue;
            char candidate[Z_NAME_MAX] = {0};
            copy_noun_words(full, first, count, starts, lengths, candidate);
            if (probe_action_form(action, value, candidate, reply)) {
                append_text(chosen, Z_NAME_MAX, candidate);
                return true;
            }
        }
    }
    return false;
}

static bool reply_says_climb_direction_fails(const char *reply, bool upward) {
    const char *word = upward ? "upward" : "downward";
    const char *short_word = upward ? "up" : "down";
    return (text_has(reply, "doesn't lead") && text_has(reply, word)) ||
           (text_has(reply, "don't lead") && text_has(reply, word)) ||
           (text_has(reply, "does not lead") && text_has(reply, word)) ||
           (text_has(reply, "doesn't go") && text_has(reply, short_word)) ||
           (text_has(reply, "don't go") && text_has(reply, short_word)) ||
           (text_has(reply, "does not go") && text_has(reply, short_word));
}

static const Action inspect[] = {
    OBJECT("Look under", "look under"), OBJECT("Look inside", "look in"),
    OBJECT("Search", "search"), OBJECT("Smell", "smell"), OBJECT("Listen to", "listen to"),
    OBJECT("Read", "read")
};
static const Action handle[] = {
    OBJECT("Push", "push"), OBJECT("Pull", "pull"), OBJECT("Turn", "turn"),
    OBJECT("Lift", "lift"), OBJECT("Lower", "lower"), OBJECT("Shake", "shake"),
    OBJECT("Touch", "touch"), OBJECT("Knock on", "knock on"), OBJECT("Ring", "ring"),
    OBJECT("Wind", "wind"), OBJECT("Wake", "wake"), OBJECT("Grab", "grab"),
    OBJECT("Press", "press"), OBJECT("Tie", "tie"), OBJECT("Untie", "untie"),
    OBJECT("Open", "open"), OBJECT("Close", "close"), OBJECT("Get", "get")
};
static const Action use[] = {
    OBJECT("Light", "light"), OBJECT("Extinguish", "extinguish"), OBJECT("Wave", "wave"),
    OBJECT("Fill", "fill"), OBJECT("Kiss", "kiss"), OBJECT("Wear", "wear"),
    OBJECT("Remove", "remove"), OBJECT("Eat", "eat"), OBJECT("Drink", "drink"),
    OBJECT("Drink from", "drink from"),
    OBJECT("Take", "take"), OBJECT("Drop", "drop")
};
static const Action with[] = {
    TARGET("Put in...", "put", "in"), TARGET("Put on...", "put", "on"),
    TARGET("Put under...", "put", "under"), TARGET("Give to...", "give", "to"),
    TARGET("Throw at...", "throw", "at"), TARGET("Throw in...", "throw", "in"),
    TARGET("Tie to...", "tie", "to"), SELF("Pour on...", "pour", "on"),
    TARGET("Pour in...", "pour", "in"), SELF("Spray on...", "spray", "on"),
    SELF("Apply to...", "apply", "to"), TARGET("Wave at...", "wave", "at")
};
static const Action tools[] = {
    TARGET("Break with...", "break", "with"), TARGET("Open with...", "open", "with"),
    TARGET("Unlock with...", "unlock", "with"),
    TARGET("Inflate with...", "inflate", "with"), TARGET("Light with...", "light", "with"),
    TARGET("Burn with...", "burn", "with"), TARGET("Dig with...", "dig", "with"),
    TARGET("Turn with...", "turn", "with"), TARGET("Attack with...", "attack", "with"),
    TARGET("Patch with...", "plug", "with")
};
static const Action travel[] = {
    OBJECT("Climb", "climb"), OBJECT("Enter", "enter"),
    OBJECT("Walk through", "walk through"), OBJECT("Cross", "cross"), OBJECT("Get out of", "get out of"),
    SENTENCE("Stand up", "stand"), SENTENCE("Land", "land"),
    DIRECTION("Push toward...", "push")
};
static const Action robot[] = {
    DIRECTION("Go toward...", "robot, go"), SENTENCE("Lift cage", "robot, lift cage"),
    SENTENCE("Push round button", "robot, push round button"),
    SENTENCE("Push square button", "robot, push square button"),
    SENTENCE("Push triangle button", "robot, push triangular button")
};
static const Action master[] = {
    DIRECTION("Go toward...", "master, go"), SENTENCE("Follow me", "master, follow me"),
    SENTENCE("Stay here", "master, stay"),
    SENTENCE("Open door", "master, open door"), SENTENCE("Close door", "master, close door"),
    SENTENCE("Push button", "master, push button"),
    {"Turn dial...", "master, turn dial to", NULL, PICK_DIGIT}
};
static const Action demon[] = {
    {"Give me...", "demon, give me", NULL, PICK_TARGET},
    {"Move...", "demon, move", NULL, PICK_TARGET},
    {"Take...", "demon, take", NULL, PICK_TARGET},
    {"Attack...", "demon, attack", NULL, PICK_TARGET}
};

static unsigned copy_actions(Action list[MAX_ACTIONS], const Action *from, unsigned count) {
    for (unsigned i = 0; i < count; i++) list[i] = from[i];
    return count;
}

#define COPY(group) copy_actions(list, group, sizeof(group) / sizeof(group[0]))

static Action climb_action(unsigned id) {
    unsigned room = z_global(0);
    // Keep the established ladder and mine-stair directions where the story
    // has explicit route geometry; probe the story parser for other climbables.
    if (id == ZORK1_MINE_LADDER && room == ZORK1_LADDER_TOP)
        return (Action)OBJECT("Climb down", "climb down");
    if ((id == ZORK1_MINE_LADDER && room == ZORK1_LADDER_BOTTOM) ||
        (id == ZORK1_STAIRS && (room == ZORK1_LADDER_TOP || room == ZORK1_GAS_ROOM)))
        return (Action)OBJECT("Climb up", "climb up");

    Action generic = travel[0];
    char noun[Z_NAME_MAX], reply[ACTION_REPLY_SIZE];
    if (!resolve_action_noun(generic, id, NULL, noun, reply))
        return (Action){NULL, NULL, NULL, DO_OBJECT};

    bool choose_down = reply_says_climb_direction_fails(reply, true);
    bool choose_up = reply_says_climb_direction_fails(reply, false);
    if (!choose_down && !choose_up) {
        Action up = OBJECT("Climb up", "climb up");
        Action down = OBJECT("Climb down", "climb down");
        char up_reply[ACTION_REPLY_SIZE], down_reply[ACTION_REPLY_SIZE];
        bool up_known = resolve_action_noun(up, id, NULL, noun, up_reply);
        bool down_known = resolve_action_noun(down, id, NULL, noun, down_reply);
        bool up_blocked = up_known && reply_says_climb_direction_fails(up_reply, true);
        bool down_blocked = down_known && reply_says_climb_direction_fails(down_reply, false);
        if (up_blocked != down_blocked) {
            choose_down = up_blocked;
            choose_up = down_blocked;
        }
    }

    if (choose_down != choose_up) {
        Action direction = choose_down ? (Action)OBJECT("Climb down", "climb down")
                                       : (Action)OBJECT("Climb up", "climb up");
        if (resolve_action_noun(direction, id, NULL, noun, reply)) return direction;
    }
    return generic;
}

unsigned story_actions(unsigned chapter, unsigned id, bool carried, ActionGroup group, Action list[MAX_ACTIONS]) {
    char name[Z_NAME_MAX];
    story_noun(id, name);
    switch (group) {
        case INSPECT: return COPY(inspect);
        case HANDLE: return COPY(handle);
        case USE: return COPY(use);
        case COMBINE: return COPY(with);
        case TOOLS: return COPY(tools);
        case MOVE: {
            unsigned count = COPY(travel);
            Action suggested = climb_action(id);
            list[0] = suggested.verb ? suggested : travel[0];
            return count;
        }
        case ORDERS:
            if (chapter == 1 && text_equal(name, "robot")) {
                if (z_global(0) == 106) return copy_actions(list, robot + 1, 1);
                unsigned count = 0;
                list[count++] = robot[0];
                if (z_global(0) == 87) {
                    for (unsigned i = 2; i < sizeof(robot) / sizeof(robot[0]); i++) list[count++] = robot[i];
                }
                return count;
            }
            if (text_equal(name, "dungeon master")) return COPY(master);
            if (text_equal(name, "demon")) return COPY(demon);
            return 0;
        default: break;
    }

    unsigned count = 0;
    // Conditional actions stop one short of MAX_ACTIONS so "More actions..." always fits.
    #define ADD(action) do { if (count < MAX_ACTIONS - 1) list[count++] = (Action)action; } while (0)
    ADD(OBJECT("Examine", "examine"));
    if ((chapter == 1 && (text_equal(name, "robot") || text_equal(name, "demon"))) ||
        (chapter == 2 && text_equal(name, "dungeon master")))
        ADD(MENU("Give orders...", SHOW_ORDERS));
    if (chapter == 2 && id == 88)
        ADD(NUMBER("Turn dial...", "turn dial to", PICK_DIGIT));
    if (chapter == 2 && id == 129) ADD(OBJECT("Take", "take"));
    if (chapter == 2 && id == 76)
        ADD(NUMBER("Set dial...", "set dial to", PICK_YEAR));
    if (chapter == 2 && id == 122) {
        if (z_parent(chapters[chapter].player) == id) ADD(OBJECT("Get out", "get out of"));
        else ADD(OBJECT("Enter", "enter"));
        ADD(DIRECTION("Push toward...", "push"));
    }
    if (!(chapter == 2 && id == 122) && text_has(name, "boat")) {
        if (z_parent(chapters[chapter].player) == id) ADD(OBJECT("Get out", "get out of"));
        else ADD(OBJECT("Enter", "enter"));
    }
    if (!(chapter == 2 && id == 122) && (text_has(name, "bucket") || text_has(name, "basket"))) {
        if (z_parent(chapters[chapter].player) == id) ADD(SENTENCE("Stand up", "stand"));
        else ADD(OBJECT("Enter", "enter"));
    }
    if (text_equal(name, "teapot")) ADD(OBJECT("Fill", "fill"));
    if (text_has(name, "panel") || text_has(name, "button")) ADD(OBJECT("Push", "push"));
    if (id >= TARGET_NORTH_WALL && id <= TARGET_WEST_WALL) ADD(OBJECT("Push", "push"));
    if (chapter == 2 && id == ZORK3_DUNGEON_DOOR && z_global(0) == ZORK3_DUNGEON_ENTRANCE) {
        ADD(OBJECT("Knock on", "knock on"));
    } else if (z_attr(id, ATTR_CONTAINER) || z_attr(id, ATTR_DOOR) || text_has(name, "window") || text_has(name, "door")) {
        if (z_attr(id, ATTR_OPEN)) ADD(OBJECT("Close", "close"));
        else ADD(OBJECT("Open", "open"));
        if (text_equal(name, "trap door")) ADD(SENTENCE("Descend", "down"));
    }
    if (text_has(name, "match")) ADD(OBJECT("Light", "light"));
    if (text_equal(name, "vial")) ADD(OBJECT("Drink from", "drink from"));
    if (text_equal(name, "rainbow")) ADD(OBJECT("Cross", "cross"));
    if (text_equal(name, "punctured boat")) ADD(TARGET("Patch with...", "plug", "with"));
    if (chapter == 2 && id == ZORK3_VIKING_SHIP) ADD(SENTENCE("Hail sailor", "hello sailor"));
    if (chapter == 0 && id == 157) ADD(OBJECT("Move", "move"));
    if (chapter == 0 && id == 29) {
        if (z_attr(id, ATTR_OPEN)) {
            if (z_global(0) == 216) ADD(SENTENCE("Climb up", "up"));
            else ADD(SENTENCE("Descend", "down"));
        } else ADD(TARGET("Unlock with...", "unlock", "with"));
    }
    if (chapter == 0 && id == ZORK1_TROLL) ADD(OFFER("Throw item...", "throw", "at"));
    if (chapter == 1 && id == ZORK2_DRAGON) ADD(OBJECT("Kick", "kick"));
    if (chapter == 2 && id == ZORK3_OLD_MAN) ADD(SENTENCE("Wake", "wake man"));
    if (chapter == 2 && id == ZORK3_HOODED_FIGURE && story_figure_defenseless() && z_parent(148) == id)
        ADD(SENTENCE("Remove hood", "take hood"));
    if (text_has(name, "candles")) ADD(TARGET("Light with...", "light", "with"));
    if (text_has(name, "pole") || text_has(name, "basket")) {
        ADD(OBJECT("Lift", "lift"));
        ADD(OBJECT("Lower", "lower"));
    }
    if (z_attr(id, ATTR_READABLE)) ADD(OBJECT("Read", "read"));
    if (z_attr(id, ATTR_LIGHT)) {
        if (z_attr(id, ATTR_LIT)) ADD(OBJECT("Switch off", "turn off"));
        else ADD(OBJECT("Switch on", "turn on"));
    }
    if (text_has(name, "carpet") || text_has(name, "rug")) {
        ADD(OBJECT("Look under", "look under"));
        ADD(OBJECT("Move", "move"));
    }
    if (z_attr(id, ATTR_CLIMBABLE)) ADD(climb_action(id));
    if (z_attr(id, ATTR_ACTOR)) {
        ADD(OFFER("Give item...", "give", "to"));
        ADD(TARGET("Attack with...", "attack", "with"));
    }
    if (z_attr(id, ATTR_WEAPON)) ADD(TARGET("Swing at...", "swing", "at"));
    if (z_attr(id, ATTR_EDIBLE)) ADD(OBJECT("Eat", "eat"));
    if (carried) {
        ADD(OBJECT("Drop", "drop"));
        ADD(TARGET("Put in...", "put", "in"));
    } else if (z_attr(id, ATTR_PORTABLE)) ADD(OBJECT("Take", "take"));
    list[count++] = (Action)MENU("More actions...", SHOW_CATEGORIES);
    #undef ADD
    return count;
}

unsigned story_filter_defaults(unsigned id, Action list[MAX_ACTIONS], unsigned count) {
    unsigned kept = 0;
    for (unsigned i = 0; i < count; i++) {
        Action action = list[i];
        bool accepted = true;
        char noun[Z_NAME_MAX], reply[ACTION_REPLY_SIZE];
        if (action.kind == DO_OBJECT)
            accepted = resolve_action_noun(action, id, NULL, noun, reply);
        else if (action.kind == DO_LITERAL)
            accepted = probe_action_form(action, NULL, NULL, reply);
        if (accepted) list[kept++] = action;
    }
    return kept;
}

bool action_command(Action action, unsigned object, const char *value, char line[COMMAND_SIZE]) {
    char noun[Z_NAME_MAX];
    if (!action_needs_object_noun(action)) return raw_action_command(action, NULL, value, line);
    if (resolve_action_noun(action, object, value, noun, (char[ACTION_REPLY_SIZE]){0}))
        return raw_action_command(action, noun, value, line);
    // Keep arbitrary menu actions available even when the current story parser
    // rejects every generated noun form; the story should return its own refusal.
    story_parser_noun(object, noun);
    return raw_action_command(action, noun, value, line);
}

bool speech_command(unsigned mode, const char *phrase, char line[COMMAND_SIZE]) {
    static const char *const prefix[] = {"say \"", "answer \"", "", "hello "};
    line[0] = 0;
    return mode < sizeof(prefix) / sizeof(prefix[0]) && *phrase &&
           append_text(line, COMMAND_SIZE, prefix[mode]) && append_text(line, COMMAND_SIZE, phrase) &&
           (mode >= 2 || append_text(line, COMMAND_SIZE, "\""));
}

void speech_word(unsigned index, char word[Z_WORD_MAX]) {
    z_dictionary_word(index, word);
    if (text_equal(word, "ulysse")) append_text(word, Z_WORD_MAX, "s");
    else if (text_equal(word, "odysse")) append_text(word, Z_WORD_MAX, "us");
}
