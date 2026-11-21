#include "interaction.h"

#define OBJECT(label, verb) {label, verb, NULL, DO_OBJECT}
#define TARGET(label, verb, prep) {label, verb, prep, PICK_TARGET}
#define SELF(label, verb, prep) {label, verb, prep, PICK_SELF}
#define OFFER(label, verb, prep) {label, verb, prep, PICK_ITEM}
#define SENTENCE(label, verb) {label, verb, NULL, DO_LITERAL}
#define DIRECTION(label, prefix) {label, prefix, NULL, PICK_DIRECTION}
#define MENU(label, kind) {label, NULL, NULL, kind}
#define NUMBER(label, verb, kind) {label, verb, NULL, kind}

enum { ACTION_REPLY_SIZE = 512, MAX_NOUN_WORDS = 8, ACTION_MENU_PROBE_LIMIT = 48 };

static bool action_menu_probe_limited;
static unsigned action_menu_probes_remaining;

void story_menu_probe_begin(void) {
    action_menu_probe_limited = true;
    action_menu_probes_remaining = ACTION_MENU_PROBE_LIMIT;
}

void story_menu_probe_end(void) {
    action_menu_probe_limited = false;
}

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

static bool reply_disallows_action(Action action, const char *reply) {
    char cannot[COMMAND_SIZE] = "cannot ";
    char cant[COMMAND_SIZE] = "can't ";
    return action.verb && append_text(cannot, COMMAND_SIZE, action.verb) &&
           append_text(cant, COMMAND_SIZE, action.verb) &&
           (text_has(reply, cannot) || text_has(reply, cant));
}

// Reply text reduced to its first sentence: parenthetical parser notes dropped,
// lowercased, whitespace collapsed. Used for refusal classification and reports;
// full-response signatures below decide whether two actions can be merged.
void action_reply_normalized(const char *reply, char *out, unsigned capacity) {
    if (!capacity) return;
    unsigned used = 0;
    const char *p = reply;
    while (*p == ' ' || *p == '\n' || *p == '\r') p++;
    if (*p == '(') {
        while (*p && *p != ')') p++;
        if (*p == ')') p++;
        while (*p == ' ' || *p == '\n' || *p == '\r') p++;
    }
    // Replies that open with punctuation (Zork I's boat label starts "!!!!")
    // still describe a result; start at the first alphanumeric character.
    while (*p && !((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9')))
        p++;
    if (!*p) { out[0] = 0; return; }
    while (*p && *p != '.' && *p != '!' && *p != '?' && *p != '\n' && *p != '\r') {
        char ch = *p++;
        if (ch >= 'A' && ch <= 'Z') ch = (char)(ch - 'A' + 'a');
        if (ch == ' ' && used && out[used - 1] == ' ') continue;
        if (used + 1 < capacity) out[used++] = ch;
    }
    while (used && out[used - 1] == ' ') used--;
    out[used] = 0;
}

// Fingerprint the full visible reply for de-duplication. The first-sentence
// helper above remains useful for classification, but is too lossy for deciding
// that two actions are equivalent. A reply filling the capture buffer may have
// been truncated, so it is kept unique instead of being merged.
typedef struct {
    uint32_t first, second;
    ZProbeEffect effect;
} ActionReplySignature;

static bool action_reply_signature(const char *reply, ZProbeEffect effect,
                                   ActionReplySignature *signature) {
    unsigned length = 0;
    while (reply[length]) length++;
    if (length >= ACTION_REPLY_SIZE - 1 || !effect.valid) return false;

    const char *p = reply;
    while (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t') p++;
    if (*p == '(') {
        while (*p && *p != ')') p++;
        if (*p == ')') p++;
        while (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t') p++;
    }
    // Two independent 32-bit rolling hashes avoid truncating the response and
    // avoid 64-bit multiply helpers on the freestanding ARM7 build.
    uint32_t first_hash = 2166136261u, second_hash = 0x9e3779b9u;
    bool pending_space = false, any = false;
    while (*p) {
        char ch = *p++;
        if (ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t') {
            pending_space = any;
            continue;
        }
        if (pending_space) {
            first_hash = (first_hash ^ (uint8_t)' ') * 16777619u;
            second_hash ^= (uint8_t)' ' + 0x9e3779b9u + (second_hash << 6) + (second_hash >> 2);
            pending_space = false;
        }
        if (ch >= 'A' && ch <= 'Z') ch = (char)(ch - 'A' + 'a');
        first_hash = (first_hash ^ (uint8_t)ch) * 16777619u;
        second_hash ^= (uint8_t)ch + 0x9e3779b9u + (second_hash << 6) + (second_hash >> 2);
        any = true;
    }
    if (!any) return false;
    signature->first = first_hash;
    signature->second = second_hash;
    signature->effect = effect;
    return true;
}

// Replies that acknowledge a verb but change nothing. An action whose reply
// matches one of these is not offered in the primary list.
static const char *const no_effect_patterns[] = {
    "has no effect", "reveals nothing", "fiddling with", "playing in this way",
    "you can't do that", "i don't think that", "how does one", "you must tell me how",
    "what do you want to", "first tell me what", "you're not carrying", "you don't have",
    "already closed", "already open", "securely fastened", "extremely heavy",
    "cannot be carried", "you find nothing unusual", "there is nothing special",
    "isn't sleeping", "i'd sooner kiss", "that's pretty weird", "you have lost your mind",
    "there is nothing to fill", "is it empty", "how, exactly", "why knock",
    "your bare hands", "an interesting idea", "you can't cross", "you can't look inside",
    "you hear nothing", "you can hear nothing", "nothing but dust",
    "nothing happens", "you wave the", "is closed", "is locked", "isn't open yet",
    "already in", "you're already", "you are already", "hit your head",
    "makes no sound", "already on", "already off", "already lit",
    "you already have", "a valiant attempt", "isn't notably helpful",
    "you can't move the", "securely anchored", "you can't turn that",
    "contortion", "accomplished enough juggler", "can't be done",
    "doesn't seem to work", "you can't open that", "not openable",
    "can't see how to get in from here"
};

bool action_reply_is_no_effect(const char *reply) {
    char normalized[ACTION_REPLY_SIZE];
    action_reply_normalized(reply, normalized, sizeof(normalized));
    for (unsigned i = 0; i < sizeof(no_effect_patterns) / sizeof(no_effect_patterns[0]); i++)
        if (text_has(normalized, no_effect_patterns[i])) return true;
    return false;
}

static bool reply_has_no_matching_target(const char *reply) {
    char normalized[ACTION_REPLY_SIZE];
    action_reply_normalized(reply, normalized, sizeof(normalized));
    return text_has(normalized, "you can't see any") ||
           text_has(normalized, "i don't see any") ||
           text_has(normalized, "you're not at") ||
           text_has(normalized, "you are not at");
}

static bool probe_action_form(Action action, const char *value,
                              const char *noun, char reply[ACTION_REPLY_SIZE],
                              ZProbeEffect *effect) {
    char line[COMMAND_SIZE];
    reply[0] = 0;
    if (effect) *effect = (ZProbeEffect){0, 0, false};
    if (!raw_action_command(action, noun, value, line)) return false;
    if (action_menu_probe_limited) {
        if (!action_menu_probes_remaining) return false;
        action_menu_probes_remaining--;
    }
    // A probe that crashes the story (or exceeds its budget) proves nothing;
    // treat it as rejected rather than offering the action.
    bool answered = effect ? z_probe_with_effect(line, reply, ACTION_REPLY_SIZE, 20000, effect)
                           : z_probe(line, reply, ACTION_REPLY_SIZE, 20000);
    if (!answered) {
        reply[0] = 0;
        return false;
    }
    return !parser_rejected(reply) && (!noun || !reply_has_no_matching_target(reply));
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
// "climb mountain"). Probe the actual story parser and try bounded short forms
// before exposing a generated default action.
static bool resolve_action_noun(Action action, unsigned object, const char *value,
                                char chosen[Z_NAME_MAX], char reply[ACTION_REPLY_SIZE],
                                ZProbeEffect *effect) {
    char full[Z_NAME_MAX];
    chosen[0] = 0;
    story_parser_noun(object, full);
    if (!*full) return false;
    if (probe_action_form(action, value, full, reply, effect)) {
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
    if (words < 2) return false;
    // A display name can contain adjectives or trailing descriptions around
    // the parser's headword ("glass flask with liquid" -> "flask"). Try each
    // bounded single word, then the final two words for multiword headwords.
    for (unsigned first = 0; first < words; first++) {
        char candidate[Z_NAME_MAX] = {0};
        copy_noun_words(full, first, 1, starts, lengths, candidate);
        if (probe_action_form(action, value, candidate, reply, effect)) {
            append_text(chosen, Z_NAME_MAX, candidate);
            return true;
        }
    }
    if (words > 2) {
        char candidate[Z_NAME_MAX] = {0};
        copy_noun_words(full, words - 2, 2, starts, lengths, candidate);
        if (probe_action_form(action, value, candidate, reply, effect)) {
            append_text(chosen, Z_NAME_MAX, candidate);
            return true;
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
    if (!resolve_action_noun(generic, id, NULL, noun, reply, NULL))
        return (Action){NULL, NULL, NULL, DO_OBJECT};

    // A generic climb reply can be misleading (Zork III's endless stair reports
    // "don't lead upward" even though climbing up works), so verify both
    // directions instead of trusting it.
    Action up = OBJECT("Climb up", "climb up");
    Action down = OBJECT("Climb down", "climb down");
    char up_reply[ACTION_REPLY_SIZE], down_reply[ACTION_REPLY_SIZE];
    bool up_known = resolve_action_noun(up, id, NULL, noun, up_reply, NULL);
    bool down_known = resolve_action_noun(down, id, NULL, noun, down_reply, NULL);
    bool up_blocked = !up_known || reply_says_climb_direction_fails(up_reply, true) ||
                      action_reply_is_no_effect(up_reply);
    bool down_blocked = !down_known || reply_says_climb_direction_fails(down_reply, false) ||
                        action_reply_is_no_effect(down_reply);
    // Prefer the direction the generic reply points at, but never when that
    // direction is actually blocked (Zork III's endless stair is the known case).
    bool hint_down = reply_says_climb_direction_fails(reply, true);
    bool hint_up = reply_says_climb_direction_fails(reply, false);
    if (hint_down && !down_blocked) return down;
    if (hint_up && !up_blocked) return up;
    if (hint_down && !up_blocked) return up;
    if (hint_up && !down_blocked) return down;
    if (!hint_down && !hint_up && up_blocked != down_blocked) return down_blocked ? up : down;
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
        if (z_attr(id, ATTR_OPEN)) {
            ADD(OBJECT("Close", "close"));
            if (z_attr(id, ATTR_DOOR) || text_has(name, "window"))
                ADD(OBJECT("Enter", "enter"));
        } else ADD(OBJECT("Open", "open"));
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

// Single-noun verbs probed for every object, in menu priority order. The
// object's own suggested actions are probed first by the loop below; this list
// adds effectful actions the hardcoded rules never proposed.
static const struct { const char *label, *verb; } probe_verbs[] = {
    {"Take", "take"}, {"Drop", "drop"}, {"Open", "open"}, {"Close", "close"},
    {"Read", "read"}, {"Eat", "eat"}, {"Drink", "drink"}, {"Light", "light"},
    {"Extinguish", "extinguish"}, {"Fill", "fill"}, {"Wind", "wind"}, {"Ring", "ring"},
    {"Knock on", "knock on"}, {"Climb", "climb"}, {"Enter", "enter"}, {"Cross", "cross"},
    {"Push", "push"}, {"Pull", "pull"}, {"Turn", "turn"}, {"Lift", "lift"},
    {"Lower", "lower"}, {"Move", "move"}, {"Press", "press"}, {"Rub", "rub"},
    {"Shake", "shake"}, {"Touch", "touch"}, {"Wave", "wave"}, {"Search", "search"},
    {"Smell", "smell"}, {"Listen to", "listen to"}, {"Look under", "look under"},
    {"Look inside", "look in"}
};

// A few story verbs are only implemented for specific kinds of object and
// dereference a missing object otherwise (Zork I's RUB on the brass bell is a
// known crash). Probe such verbs only where the story supports them.
static bool probe_verb_allowed(const char *verb, unsigned id) {
    // Zork I's RUB/TOUCH routine dereferences a missing object for objects that
    // do not implement it (the brass bell is a known crash). The probe guard
    // recovers if another verb/object pair does the same, but skip the known one.
    if (text_equal(verb, "rub") || text_equal(verb, "touch")) return z_attr(id, ATTR_LIGHT);
    return true;
}

static bool list_has_verb(const Action *list, unsigned count, const char *verb) {
    for (unsigned i = 0; i < count; i++)
        if (list[i].verb && text_equal(list[i].verb, verb)) return true;
    return false;
}

unsigned story_filter_defaults(unsigned id, Action list[MAX_ACTIONS], unsigned count) {
    bool has_menu = count && list[count - 1].kind == SHOW_CATEGORIES;
    if (has_menu) count--;
    Action kept[MAX_ACTIONS];
    char norm[MAX_ACTIONS][48];   // First sentence, for unavailable-here classification.
    ActionReplySignature signatures[MAX_ACTIONS] = {{0}};
    bool signature_valid[MAX_ACTIONS] = {false};
    char noun_hint[Z_NAME_MAX] = {0};
    unsigned kept_count = 0, norm_count = 0;
    for (unsigned i = 0; i < count; i++) {
        Action action = list[i];
        bool accepted = true;
        char noun[Z_NAME_MAX], reply[ACTION_REPLY_SIZE];
        reply[0] = 0;
        // Examine is always offered: its reply describes the object and must not
        // be mistaken for a no-effect refusal ("The door is closed."). Its full
        // reply and observed state change participate in duplicate detection.
        if (action.verb && text_equal(action.verb, "examine")) {
            char examine_reply[ACTION_REPLY_SIZE];
            ZProbeEffect effect = {0, 0, false};
            if (resolve_action_noun(action, id, NULL, noun, examine_reply, &effect)) {
                append_text(noun_hint, sizeof(noun_hint), noun);
                action_reply_normalized(examine_reply, norm[norm_count], sizeof(norm[0]));
                signature_valid[norm_count] = action_reply_signature(examine_reply, effect, &signatures[norm_count]);
            } else norm[norm_count][0] = 0;
            kept[kept_count++] = action;
            norm_count++;
            continue;
        }
        ZProbeEffect effect = {0, 0, false};
        if (action.kind == DO_OBJECT) {
            noun[0] = 0;
            accepted = noun_hint[0] && probe_action_form(action, NULL, noun_hint, reply, &effect);
            if (accepted) append_text(noun, sizeof(noun), noun_hint);
            else accepted = resolve_action_noun(action, id, NULL, noun, reply, &effect);
            if (accepted && (reply_disallows_action(action, reply) || action_reply_is_no_effect(reply)))
                accepted = false;
        } else if (action.kind == DO_LITERAL) {
            accepted = probe_action_form(action, NULL, NULL, reply, &effect);
            if (accepted && reply_disallows_action(action, reply)) accepted = false;
        }
        if (!accepted) continue;
        kept[kept_count] = action;
        if (action.kind == DO_OBJECT || action.kind == DO_LITERAL) {
            action_reply_normalized(reply, norm[norm_count], sizeof(norm[0]));
            signature_valid[norm_count] = action_reply_signature(reply, effect, &signatures[norm_count]);
        } else norm[norm_count][0] = 0;
        kept_count++;
        norm_count++;
    }
    for (unsigned i = 0; i < sizeof(probe_verbs) / sizeof(probe_verbs[0]); i++) {
        if (kept_count >= MAX_ACTIONS - 2) break; // Reserve the final slot for the menu.
        if (list_has_verb(kept, kept_count, probe_verbs[i].verb)) continue;
        // A directional climb already chosen for this object covers the generic verb.
        if (text_equal(probe_verbs[i].verb, "climb") &&
            (list_has_verb(kept, kept_count, "climb up") || list_has_verb(kept, kept_count, "climb down"))) continue;
        if (!probe_verb_allowed(probe_verbs[i].verb, id)) continue;
        Action action = {probe_verbs[i].label, probe_verbs[i].verb, NULL, DO_OBJECT};
        char noun[Z_NAME_MAX], reply[ACTION_REPLY_SIZE];
        noun[0] = 0;
        ZProbeEffect effect = {0, 0, false};
        bool resolved = noun_hint[0] && probe_action_form(action, NULL, noun_hint, reply, &effect);
        if (resolved) append_text(noun, sizeof(noun), noun_hint);
        else resolved = resolve_action_noun(action, id, NULL, noun, reply, &effect);
        if (!resolved) continue;
        if (!reply[0] || reply_disallows_action(action, reply) || action_reply_is_no_effect(reply)) continue;
        kept[kept_count] = action;
        action_reply_normalized(reply, norm[norm_count], sizeof(norm[0]));
        signature_valid[norm_count] = action_reply_signature(reply, effect, &signatures[norm_count]);
        kept_count++;
        norm_count++;
    }
    // Collapse actions only when the full captured replies and observed story
    // state changes match, keeping the earliest. Replies that only mean "not
    // available here" are not results.
    unsigned out_count = 0;
    bool output_groupable[MAX_ACTIONS] = {false};
    for (unsigned i = 0; i < kept_count; i++) {
        bool groupable = signature_valid[i] && norm[i][0] &&
            !text_has(norm[i], "you can't see any") && !text_has(norm[i], "you're not at") &&
            !text_has(norm[i], "there is no ");
        bool duplicate = false;
        if (groupable) {
            for (unsigned j = 0; j < out_count; j++)
                if (output_groupable[j] && signatures[j].first == signatures[i].first &&
                    signatures[j].second == signatures[i].second &&
                    signatures[j].effect.first == signatures[i].effect.first &&
                    signatures[j].effect.second == signatures[i].effect.second) {
                    duplicate = true;
                    break;
                }
        }
        if (duplicate) continue;
        if (out_count != i) {
            kept[out_count] = kept[i];
            norm[out_count][0] = 0;
            if (groupable) append_text(norm[out_count], sizeof(norm[0]), norm[i]);
            signatures[out_count] = signatures[i];
            signature_valid[out_count] = signature_valid[i];
        }
        output_groupable[out_count] = groupable;
        if (!groupable) {
            norm[out_count][0] = 0;
            signatures[out_count] = (ActionReplySignature){0};
            signature_valid[out_count] = false;
        }
        out_count++;
    }
    // Keep the list within the menu budget; the last slot stays for "More actions...".
    unsigned budget = has_menu ? MAX_ACTIONS - 2 : MAX_ACTIONS - 1;
    while (out_count > budget && out_count > 0) out_count--;
    if (has_menu) kept[out_count++] = (Action)MENU("More actions...", SHOW_CATEGORIES);
    for (unsigned i = 0; i < out_count; i++) list[i] = kept[i];
    return out_count;
}

bool action_command(Action action, unsigned object, const char *value, char line[COMMAND_SIZE]) {
    char noun[Z_NAME_MAX];
    if (!action_needs_object_noun(action)) return raw_action_command(action, NULL, value, line);
    if (resolve_action_noun(action, object, value, noun, (char[ACTION_REPLY_SIZE]){0}, NULL))
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
