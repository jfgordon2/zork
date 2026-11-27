#include "interaction.h"

#define OBJECT(label, verb) {label, verb, NULL, DO_OBJECT}
#define TARGET(label, verb, prep) {label, verb, prep, PICK_TARGET}
#define SELF(label, verb, prep) {label, verb, prep, PICK_SELF}
#define OFFER(label, verb, prep) {label, verb, prep, PICK_ITEM}
#define SENTENCE(label, verb) {label, verb, NULL, DO_LITERAL}
#define DIRECTION(label, prefix) {label, prefix, NULL, PICK_DIRECTION}
#define MENU(label, kind) {label, NULL, NULL, kind}
#define NUMBER(label, verb, kind) {label, verb, NULL, kind}

enum {
    ACTION_REPLY_SIZE = 512,
    MAX_NOUN_WORDS = 8,
    ACTION_MENU_PROBE_LIMIT = 48,
    ACTION_MENU_EFFECT_PROBE_LIMIT = 24,
    ACTION_REFERENCE_PROBE_BUDGET = 20000
};

static bool action_menu_probe_limited;
static unsigned action_menu_probes_remaining;
static unsigned action_menu_probes_used_count;
static unsigned action_menu_effect_probes_remaining;
static unsigned action_menu_effect_probes_used_count;
static unsigned action_menu_probe_budget;
static ZProbeEffect action_probe_effect;
static bool action_probe_effect_valid;

void story_menu_probe_begin(void) {
    action_menu_probe_limited = true;
    action_menu_probes_remaining = ACTION_MENU_PROBE_LIMIT;
    action_menu_probes_used_count = 0;
    action_menu_effect_probes_remaining = ACTION_MENU_EFFECT_PROBE_LIMIT;
    action_menu_effect_probes_used_count = 0;
    action_menu_probe_budget = ACTION_REFERENCE_PROBE_BUDGET;
    action_probe_effect_valid = false;
    z_probe_batch_begin();
}

void story_menu_probe_end(void) {
    action_menu_probe_limited = false;
    z_probe_batch_end();
}

unsigned story_menu_effect_probes_used(void) {
    return action_menu_effect_probes_used_count;
}

unsigned story_menu_probes_used(void) {
    return action_menu_probes_used_count;
}

void story_menu_probe_set_budget(unsigned budget) {
    if (budget) action_menu_probe_budget = budget;
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
    bool valid;
} ActionReplySignature;

static bool action_reply_signature(const char *reply, ActionReplySignature *signature) {
    unsigned length = 0;
    while (reply[length]) length++;
    if (length >= ACTION_REPLY_SIZE - 1) return false;

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
    signature->valid = true;
    return true;
}

static bool action_reply_signatures_match(ActionReplySignature a, ActionReplySignature b) {
    return a.valid && b.valid && a.first == b.first && a.second == b.second;
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
    "can't see how to get in from here", "you should say whether you want to go up or down",
    "the stairs are endless", "doesn't look like he'll let you past",
    "climbing the walls is to no avail", "doesn't lead upward", "don't lead upward",
    "does not lead upward", "doesn't lead downward", "don't lead downward",
    "does not lead downward", "you can't go that way", "you cannot go that way"
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

// Noun resolution can retry shorter display-name forms after the parser rejects
// a phrase or fails to find the named target. Callers that already tried the
// full noun reuse this helper to avoid repeating that same probe.
static bool reply_allows_noun_fallback(const char *reply) {
    return parser_rejected(reply) || reply_has_no_matching_target(reply);
}

static bool probe_action_form(Action action, const char *value,
                              const char *noun, char reply[ACTION_REPLY_SIZE]) {
    char line[COMMAND_SIZE];
    reply[0] = 0;
    if (!raw_action_command(action, noun, value, line)) return false;
    if (action_menu_probe_limited) {
        if (!action_menu_probes_remaining) return false;
        action_menu_probes_remaining--;
        action_menu_probes_used_count++;
    }
    // A probe that crashes the story (or exceeds its budget) proves nothing;
    // treat it as rejected rather than offering the action.
    unsigned budget = action_menu_probe_limited ? action_menu_probe_budget : ACTION_REFERENCE_PROBE_BUDGET;
    bool answered = action_menu_probe_limited
        ? z_probe_menu(line, reply, ACTION_REPLY_SIZE, budget)
        : z_probe(line, reply, ACTION_REPLY_SIZE, budget);
    if (!answered) {
        reply[0] = 0;
        return false;
    }
    return !parser_rejected(reply) && (!noun || !reply_has_no_matching_target(reply));
}

static bool action_effect_signature(Action action, const char *noun,
                                    ActionReplySignature expected, ZProbeEffect *effect) {
    char line[COMMAND_SIZE], reply[ACTION_REPLY_SIZE];
    if (!expected.valid || !raw_action_command(action,
            action_needs_object_noun(action) ? noun : NULL, NULL, line)) return false;
    if (action_menu_probe_limited) {
        if (!action_menu_effect_probes_remaining) return false;
        action_menu_effect_probes_remaining--;
        action_menu_effect_probes_used_count++;
    }
    ZProbeEffect observed_effect = {0, 0, false};
    unsigned budget = action_menu_probe_limited ? action_menu_probe_budget : ACTION_REFERENCE_PROBE_BUDGET;
    if (!z_probe_with_effect(line, reply, sizeof(reply), budget, &observed_effect)) return false;
    ActionReplySignature observed_reply = {0};
    if (!action_reply_signature(reply, &observed_reply) ||
        !action_reply_signatures_match(expected, observed_reply) || !observed_effect.valid) return false;
    *effect = observed_effect;
    return true;
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

static bool resolve_action_noun_short_forms(Action action, const char *value,
                                            const char *full, char chosen[Z_NAME_MAX],
                                            char reply[ACTION_REPLY_SIZE]) {
    unsigned starts[MAX_NOUN_WORDS], lengths[MAX_NOUN_WORDS], words = 0;
    for (unsigned i = 0; full[i] && words < MAX_NOUN_WORDS;) {
        while (full[i] == ' ') i++;
        if (!full[i]) break;
        starts[words] = i;
        while (full[i] && full[i] != ' ') i++;
        lengths[words] = i - starts[words];
        words++;
    }
    if (words < 2 || !reply_allows_noun_fallback(reply)) return false;
    // A display name can contain adjectives or trailing descriptions around
    // the parser's headword. Try each bounded single word, then the final pair.
    for (unsigned first = 0; first < words; first++) {
        char candidate[Z_NAME_MAX] = {0};
        copy_noun_words(full, first, 1, starts, lengths, candidate);
        if (probe_action_form(action, value, candidate, reply)) {
            append_text(chosen, Z_NAME_MAX, candidate);
            return true;
        }
        if (!reply_allows_noun_fallback(reply)) return false;
    }
    if (words > 2) {
        char candidate[Z_NAME_MAX] = {0};
        copy_noun_words(full, words - 2, 2, starts, lengths, candidate);
        if (probe_action_form(action, value, candidate, reply)) {
            append_text(chosen, Z_NAME_MAX, candidate);
            return true;
        }
    }
    return false;
}

// Some Z-machine grammars accept an object's short headword in an action but
// reject its full display name (for example, "climb mountain range" versus
// "climb mountain"). Probe the actual story parser and try bounded short forms
// before exposing a generated default action.
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
    return resolve_action_noun_short_forms(action, value, full, chosen, reply);
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

static bool reply_blocks_vertical_route(const char *reply) {
    char normalized[ACTION_REPLY_SIZE];
    action_reply_normalized(reply, normalized, sizeof(normalized));
    return text_has(normalized, "you can't go that way") ||
           text_has(normalized, "you cannot go that way") ||
           text_has(normalized, "you can't reach") ||
           text_has(normalized, "you cannot reach") ||
           text_has(normalized, "is closed") ||
           text_has(normalized, "the stairs are endless") ||
           text_has(normalized, "only santa claus climbs down chimneys") ||
           text_has(normalized, "doesn't look like he'll let you past") ||
           text_has(normalized, "climbing the walls is to no avail");
}

static bool is_vertical_route_action(Action action) {
    return action.verb && (text_equal(action.verb, "up") || text_equal(action.verb, "down"));
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

#include "action_profiles.inc"

enum { VERTICAL_ROUTE_UP = 1, VERTICAL_ROUTE_DOWN = 2 };

static uint32_t find_action_object_candidate_mask(unsigned game, unsigned id) {
    unsigned low = 0, high = sizeof(action_candidate_masks) / sizeof(action_candidate_masks[0]);
    while (low < high) {
        unsigned mid = low + (high - low) / 2;
        const ActionCandidateMask *entry = &action_candidate_masks[mid];
        if (entry->game < game || (entry->game == game && entry->object < id)) low = mid + 1;
        else high = mid;
    }
    if (low < sizeof(action_candidate_masks) / sizeof(action_candidate_masks[0]) &&
        action_candidate_masks[low].game == game && action_candidate_masks[low].object == id) {
        return action_candidate_masks[low].verbs;
    }
    // An object omitted from the audit data must not silently lose verbs.
    return UINT32_MAX;
}

static uint32_t action_object_attributes(unsigned id) {
    uint32_t attributes = 0;
    for (unsigned attribute = 0; attribute < 32; attribute++)
        if (z_attr(id, attribute)) attributes |= 1u << attribute;
    return attributes;
}

// Route masks are generated from interpreter probes over the full route audit.
// Unknown states retain the existing explicit-exit fallback.
static bool find_action_vertical_route_mask(unsigned game, unsigned id, bool carried,
                                            uint8_t *directions) {
    unsigned room = z_global(0);
    unsigned parent = z_parent(id);
    uint32_t attributes = action_object_attributes(id);
    unsigned low = 0, high = ACTION_VERTICAL_ROUTE_MASK_COUNT;
    while (low < high) {
        unsigned mid = low + (high - low) / 2;
        const ActionVerticalRouteMask *entry = &action_vertical_route_masks[mid];
        bool before;
        if (entry->game != game) before = entry->game < game;
        else if (entry->room != room) before = entry->room < room;
        else if (entry->object != id) before = entry->object < id;
        else if (entry->parent != parent) before = entry->parent < parent;
        else if (entry->carried != (unsigned)carried)
            before = entry->carried < (unsigned)carried;
        else before = entry->attributes < attributes;
        if (before) low = mid + 1;
        else high = mid;
    }
    if (low < ACTION_VERTICAL_ROUTE_MASK_COUNT) {
        const ActionVerticalRouteMask *entry = &action_vertical_route_masks[low];
        if (entry->game == game && entry->room == room && entry->object == id &&
            entry->parent == parent && entry->carried == (unsigned)carried &&
            entry->attributes == attributes) {
            *directions = entry->directions;
            return true;
        }
    }
    return false;
}

// State-specific entries are only used for an exact observed structural key.
// A missed key retains the broader per-object superset; a wholly unprofiled
// object falls back to all candidate verbs. Route evidence must cover hidden
// story-flag branches that can change actions without changing this key.
static uint32_t find_action_candidate_mask(unsigned game, unsigned id, bool carried) {
    unsigned room = z_global(0);
    unsigned parent = z_parent(id);
    uint32_t attributes = action_object_attributes(id);
    unsigned low = 0, high = sizeof(action_candidate_state_masks) /
                         sizeof(action_candidate_state_masks[0]);
    while (low < high) {
        unsigned mid = low + (high - low) / 2;
        const ActionCandidateStateMask *entry = &action_candidate_state_masks[mid];
        bool before;
        if (entry->game != game) before = entry->game < game;
        else if (entry->room != room) before = entry->room < room;
        else if (entry->object != id) before = entry->object < id;
        else if (entry->parent != parent) before = entry->parent < parent;
        else if (entry->carried != (unsigned)carried)
            before = entry->carried < (unsigned)carried;
        else before = entry->attributes < attributes;
        if (before) low = mid + 1;
        else high = mid;
    }
    if (low < sizeof(action_candidate_state_masks) / sizeof(action_candidate_state_masks[0])) {
        const ActionCandidateStateMask *entry = &action_candidate_state_masks[low];
        if (entry->game == game && entry->room == room && entry->object == id &&
            entry->parent == parent && entry->carried == (unsigned)carried &&
            entry->attributes == attributes)
            return entry->verbs;
    }
    return find_action_object_candidate_mask(game, id);
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

    // A generic climb reply can be misleading (Zork III's endless stair reports
    // "don't lead upward" even though climbing up works), so verify both
    // directions instead of trusting it.
    Action up = OBJECT("Climb up", "climb up");
    Action down = OBJECT("Climb down", "climb down");
    char up_reply[ACTION_REPLY_SIZE], down_reply[ACTION_REPLY_SIZE];
    bool up_known = resolve_action_noun(up, id, NULL, noun, up_reply);
    bool down_known = resolve_action_noun(down, id, NULL, noun, down_reply);
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

static Action climb_action_fast(unsigned id) {
    unsigned room = z_global(0);
    if (id == ZORK1_MINE_LADDER && room == ZORK1_LADDER_TOP)
        return (Action)OBJECT("Climb down", "climb down");
    if ((id == ZORK1_MINE_LADDER && room == ZORK1_LADDER_BOTTOM) ||
        (id == ZORK1_STAIRS && (room == ZORK1_LADDER_TOP || room == ZORK1_GAS_ROOM)))
        return (Action)OBJECT("Climb up", "climb up");
    return travel[0];
}

static bool is_stair_or_ladder(unsigned id) {
    char name[Z_NAME_MAX];
    story_noun(id, name);
    return text_has(name, "stair") || text_has(name, "ladder") || text_has(name, "step");
}

// Make only proven vertical routes available from a stair or ladder target.
// Audited states use interpreter-derived masks; unprofiled states fall back to
// explicit room exits, which are checked again by the action filter.
static unsigned append_vertical_route_actions(unsigned chapter, unsigned id, bool carried,
                                              Action list[MAX_ACTIONS], unsigned count) {
    if (carried || !is_stair_or_ladder(id)) return 0;
    unsigned added = 0;
    uint8_t directions = 0;
    bool profiled = find_action_vertical_route_mask(chapter, id, carried, &directions);
    if (!profiled) {
        const uint8_t *exit;
        unsigned room = z_global(0);
        if (z_property(room, path_properties[8], &exit)) directions |= VERTICAL_ROUTE_UP;
        if (z_property(room, path_properties[9], &exit)) directions |= VERTICAL_ROUTE_DOWN;
    }
    if (directions & VERTICAL_ROUTE_UP)
        list[count + added++] = (Action)SENTENCE("Go up", "up");
    if (directions & VERTICAL_ROUTE_DOWN)
        list[count + added++] = (Action)SENTENCE("Go down", "down");
    return added;
}

static unsigned story_actions_internal(unsigned chapter, unsigned id, bool carried,
                                       ActionGroup group, Action list[MAX_ACTIONS],
                                       bool probe_climb_direction) {
    char name[Z_NAME_MAX];
    story_noun(id, name);
    switch (group) {
        case INSPECT: return COPY(inspect);
        case HANDLE: return COPY(handle);
        case USE: return COPY(use);
        case COMBINE: return COPY(with);
        case TOOLS: return COPY(tools);
        case MOVE: {
            unsigned count = append_vertical_route_actions(chapter, id, carried, list, 0);
            if (count) {
                // The direct route is clearer than the generic "Climb" entry;
                // keep the other movement verbs available after it.
                for (unsigned i = 1; i < sizeof(travel) / sizeof(travel[0]); i++)
                    list[count++] = travel[i];
                return count;
            }
            uint8_t profiled_directions = 0;
            if (!carried && is_stair_or_ladder(id) &&
                find_action_vertical_route_mask(chapter, id, carried, &profiled_directions)) {
                count = 0;
                for (unsigned i = 1; i < sizeof(travel) / sizeof(travel[0]); i++)
                    list[count++] = travel[i];
                return count;
            }
            count = COPY(travel);
            Action suggested = probe_climb_direction ? climb_action(id) : climb_action_fast(id);
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
    unsigned vertical_routes = append_vertical_route_actions(chapter, id, carried, list, count);
    count += vertical_routes;
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
    // The seat is a visible focus target in the time machine rooms. The story
    // accepts GET IN SEAT and places the player inside the machine, so expose
    // that route directly when the seat is focused. While seated, the machine
    // itself remains the exit target and offers GET OUT OF GOLD MACHINE.
    if (chapter == 2 && text_equal(name, "seat") && z_parent(id) == 122 &&
        z_parent(chapters[chapter].player) != 122)
        ADD(OBJECT("Enter", "get in"));
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
    if (chapter == 2 && text_equal(name, "pedestal") &&
        (z_global(0) == ZORK3_JEWEL_PRESENT || z_global(0) == ZORK3_JEWEL_947))
        ADD(SENTENCE("Read plaque", "read plaque"));
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
    uint8_t profiled_directions = 0;
    bool vertical_profiled = !carried && is_stair_or_ladder(id) &&
        find_action_vertical_route_mask(chapter, id, carried, &profiled_directions);
    if (z_attr(id, ATTR_CLIMBABLE) && !vertical_routes && !vertical_profiled &&
        count < MAX_ACTIONS - 1)
        list[count++] = probe_climb_direction ? climb_action(id) : climb_action_fast(id);
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

unsigned story_actions(unsigned chapter, unsigned id, bool carried, ActionGroup group,
                       Action list[MAX_ACTIONS]) {
    return story_actions_internal(chapter, id, carried, group, list, true);
}

unsigned story_actions_fast(unsigned chapter, unsigned id, bool carried, ActionGroup group,
                            Action list[MAX_ACTIONS]) {
    return story_actions_internal(chapter, id, carried, group, list, false);
}

// Single-noun verbs probed for every object, in menu priority order. The
// object's own suggested actions are probed first by the loop below; this list
// adds effectful actions the hardcoded rules never proposed.
static const struct { const char *label, *verb; } probe_verbs[] = {
#define PROBE_VERB(label, verb) {label, verb},
#include "action_probe_verbs.def"
#undef PROBE_VERB
};

typedef enum { GATE_ATTRIBUTE, GATE_CARRIED, GATE_PARENT } ProbeGateKind;
typedef struct {
    const char *verb;
    ProbeGateKind kind;
    unsigned state_attribute, required_attribute;
    bool allowed_when_set;
} ProbeStateGate;

typedef struct {
    uint32_t attributes;
    unsigned player_parent, object_parent;
    uint8_t vertical_directions;
    bool carried, player_parent_known;
    bool vertical_routes_known, vertical_routes_profiled;
} ActionStateSnapshot;

// Inverse actions whose availability follows a reliable story-state signal.
// Keep this list conservative: don't infer states from names or parser replies.
static const ProbeStateGate probe_state_gates[] = {
    {"open", GATE_ATTRIBUTE, ATTR_OPEN, 0, false},
    {"close", GATE_ATTRIBUTE, ATTR_OPEN, 0, true},
    {"take", GATE_CARRIED, 0, 0, false},
    {"drop", GATE_CARRIED, 0, 0, true},
    {"light", GATE_ATTRIBUTE, ATTR_LIT, ATTR_LIGHT, false},
    {"extinguish", GATE_ATTRIBUTE, ATTR_LIT, ATTR_LIGHT, true},
    {"turn on", GATE_ATTRIBUTE, ATTR_LIT, ATTR_LIGHT, false},
    {"turn off", GATE_ATTRIBUTE, ATTR_LIT, ATTR_LIGHT, true},
    {"enter", GATE_PARENT, 0, 0, false},
    {"get out of", GATE_PARENT, 0, 0, true},
    {"stand", GATE_PARENT, 0, 0, true},
};

static ActionStateSnapshot capture_action_state(unsigned game, unsigned id, bool carried) {
    ActionStateSnapshot state = {0};
    state.attributes = action_object_attributes(id);
    state.object_parent = z_parent(id);
    state.carried = carried;
    if (game < 3) {
        state.player_parent = z_parent(chapters[game].player);
        state.player_parent_known = true;
    }
    if (!carried && is_stair_or_ladder(id)) {
        state.vertical_routes_profiled = game < 3 &&
            find_action_vertical_route_mask(game, id, carried, &state.vertical_directions);
        state.vertical_routes_known = state.vertical_routes_profiled;
        if (!state.vertical_routes_profiled) {
            const uint8_t *exit;
            unsigned room = z_global(0);
            if (z_property(room, path_properties[8], &exit))
                state.vertical_directions |= VERTICAL_ROUTE_UP;
            if (z_property(room, path_properties[9], &exit))
                state.vertical_directions |= VERTICAL_ROUTE_DOWN;
            state.vertical_routes_known = state.vertical_directions != 0;
        }
    }
    return state;
}

static bool vertical_route_action_prevalidated(const ActionStateSnapshot *state, Action action) {
    if (!state->vertical_routes_profiled || !is_vertical_route_action(action)) return false;
    if (text_equal(action.verb, "up")) return (state->vertical_directions & VERTICAL_ROUTE_UP) != 0;
    return (state->vertical_directions & VERTICAL_ROUTE_DOWN) != 0;
}

static bool action_state_allows(unsigned game, unsigned id, const ActionStateSnapshot *state,
                                const char *verb) {
    for (unsigned i = 0; i < sizeof(probe_state_gates) / sizeof(probe_state_gates[0]); i++) {
        const ProbeStateGate *gate = &probe_state_gates[i];
        if (!text_equal(verb, gate->verb)) continue;
        bool current_state;
        if (gate->kind == GATE_ATTRIBUTE) {
            if (gate->required_attribute &&
                !(state->attributes & (1u << gate->required_attribute))) break;
            current_state = (state->attributes & (1u << gate->state_attribute)) != 0;
        } else if (gate->kind == GATE_CARRIED) {
            current_state = state->carried;
        } else {
            if (!state->player_parent_known || game >= 3) break;
            current_state = state->player_parent == id;
        }
        return current_state == gate->allowed_when_set;
    }
    return true;
}

// Don't spend probes on actions that the object's known state rules out.
// Known route evidence likewise replaces generic climb/up/down probes.
static bool probe_verb_allowed(unsigned game, unsigned id,
                               const ActionStateSnapshot *state, const char *verb) {
    if (!action_state_allows(game, id, state, verb)) return false;
    if (text_equal(verb, "climb") && state->vertical_routes_known) return false;
    if (state->vertical_routes_profiled &&
        (text_equal(verb, "up") || text_equal(verb, "down")))
        return text_equal(verb, "up")
            ? (state->vertical_directions & VERTICAL_ROUTE_UP) != 0
            : (state->vertical_directions & VERTICAL_ROUTE_DOWN) != 0;
    // Zork I's RUB/TOUCH routine dereferences a missing object for objects that
    // do not implement it (the brass bell is a known crash). Probe only where
    // the story exposes the matching light-source attribute.
    if (text_equal(verb, "rub") || text_equal(verb, "touch"))
        return (state->attributes & (1u << ATTR_LIGHT)) != 0;
    return true;
}

static bool action_state_prevalidated(unsigned game, unsigned id,
                                      const ActionStateSnapshot *state, Action action) {
    (void)game;
    (void)id;
    return vertical_route_action_prevalidated(state, action);
}

static bool list_has_verb(const Action *list, unsigned count, const char *verb) {
    for (unsigned i = 0; i < count; i++)
        if (list[i].verb && text_equal(list[i].verb, verb)) return true;
    return false;
}

static bool list_has_probe_verb(const Action *list, unsigned count, const char *verb) {
    if (list_has_verb(list, count, verb)) return true;
    if (text_equal(verb, "light")) return list_has_verb(list, count, "turn on");
    if (text_equal(verb, "extinguish")) return list_has_verb(list, count, "turn off");
    return false;
}

static bool resolve_probe_candidate(Action action, unsigned id, const char *noun_hint,
                                    char noun[Z_NAME_MAX], char reply[ACTION_REPLY_SIZE]) {
    noun[0] = 0;
    bool resolved = noun_hint[0] && probe_action_form(action, NULL, noun_hint, reply);
    if (resolved) append_text(noun, Z_NAME_MAX, noun_hint);
    else if (noun_hint[0] && reply_allows_noun_fallback(reply))
        resolved = resolve_action_noun_short_forms(action, NULL, noun_hint, noun, reply);
    else if (!noun_hint[0]) resolved = resolve_action_noun(action, id, NULL, noun, reply);
    if (action.verb && text_equal(action.verb, "climb up") &&
        reply_says_climb_direction_fails(reply, true)) return false;
    if (action.verb && text_equal(action.verb, "climb down") &&
        reply_says_climb_direction_fails(reply, false)) return false;
    if (is_vertical_route_action(action) && reply_blocks_vertical_route(reply)) return false;
    return resolved && reply[0] && !reply_disallows_action(action, reply) &&
           !action_reply_is_no_effect(reply);
}

static unsigned story_filter_defaults_internal(unsigned game, unsigned id, bool carried,
                                               Action list[MAX_ACTIONS], unsigned count,
                                               bool use_mask, uint32_t verb_mask,
                                               unsigned probe_budget) {
    ActionStateSnapshot state = capture_action_state(game, id, carried);
    unsigned old_probe_budget = action_menu_probe_budget;
    action_menu_probe_budget = probe_budget;
    bool has_menu = count && list[count - 1].kind == SHOW_CATEGORIES;
    if (has_menu) count--;
    Action kept[MAX_ACTIONS];
    char norm[MAX_ACTIONS][48];   // First sentence, for unavailable-here classification.
    char nouns[MAX_ACTIONS][Z_NAME_MAX] = {{0}};
    ActionReplySignature signatures[MAX_ACTIONS] = {{0}};
    ZProbeEffect effects[MAX_ACTIONS] = {{0}};
    bool effect_attempted[MAX_ACTIONS] = {false};
    bool effect_valid[MAX_ACTIONS] = {false};
    char noun_hint[Z_NAME_MAX] = {0};
    unsigned kept_count = 0;
    for (unsigned i = 0; i < count; i++) {
        Action action = list[i];
        bool accepted = true;
        char noun[Z_NAME_MAX], reply[ACTION_REPLY_SIZE];
        ActionReplySignature signature = {0};
        noun[0] = 0;
        reply[0] = 0;
        // Examine is always offered: its reply describes the object and must not
        // be mistaken for a no-effect refusal ("The door is closed."). Its full
        // reply and observed state change participate in duplicate detection.
        if (action.verb && text_equal(action.verb, "examine")) {
            char examine_reply[ACTION_REPLY_SIZE];
            if (resolve_action_noun(action, id, NULL, noun, examine_reply)) {
                append_text(noun_hint, sizeof(noun_hint), noun);
                action_reply_normalized(examine_reply, norm[kept_count], sizeof(norm[0]));
                action_reply_signature(examine_reply, &signature);
            } else norm[kept_count][0] = 0;
            kept[kept_count] = action;
            append_text(nouns[kept_count], sizeof(nouns[0]), noun);
            signatures[kept_count] = signature;
            kept_count++;
            continue;
        }
        if (action.verb && !probe_verb_allowed(game, id, &state, action.verb)) continue;
        if (action_state_prevalidated(game, id, &state, action)) {
            // The menu suggestion is derived from this exact story state, so
            // don't execute a state-changing probe just to rediscover it.
            kept[kept_count] = action;
            norm[kept_count][0] = 0;
            nouns[kept_count][0] = 0;
            signatures[kept_count] = (ActionReplySignature){0};
            kept_count++;
            continue;
        } else if (action.kind == DO_OBJECT) {
            accepted = noun_hint[0] && probe_action_form(action, NULL, noun_hint, reply);
            if (accepted) append_text(noun, sizeof(noun), noun_hint);
            else if (noun_hint[0] && reply_allows_noun_fallback(reply))
                accepted = resolve_action_noun_short_forms(action, NULL, noun_hint, noun, reply);
            else if (!noun_hint[0]) accepted = resolve_action_noun(action, id, NULL, noun, reply);
            if (accepted && (reply_disallows_action(action, reply) || action_reply_is_no_effect(reply)))
                accepted = false;
        } else if (action.kind == DO_LITERAL) {
            accepted = probe_action_form(action, NULL, NULL, reply);
            if (accepted && reply_disallows_action(action, reply)) accepted = false;
            if (accepted && is_vertical_route_action(action) && reply_blocks_vertical_route(reply))
                accepted = false;
        }
        if (!accepted) continue;
        kept[kept_count] = action;
        if (action.kind == DO_OBJECT || action.kind == DO_LITERAL) {
            action_reply_normalized(reply, norm[kept_count], sizeof(norm[0]));
            action_reply_signature(reply, &signature);
        } else norm[kept_count][0] = 0;
        append_text(nouns[kept_count], sizeof(nouns[0]), noun);
        signatures[kept_count] = signature;
        kept_count++;
    }
    for (unsigned i = 0; i < sizeof(probe_verbs) / sizeof(probe_verbs[0]); i++) {
        if (use_mask && !(verb_mask & (1u << i))) continue;
        if (kept_count >= MAX_ACTIONS - 2) break; // Reserve the final slot for the menu.
        if (list_has_probe_verb(kept, kept_count, probe_verbs[i].verb)) continue;
        // A directional climb already chosen for this object covers the generic verb.
        if (text_equal(probe_verbs[i].verb, "climb") &&
            (list_has_verb(kept, kept_count, "climb up") || list_has_verb(kept, kept_count, "climb down") ||
             list_has_verb(kept, kept_count, "up") || list_has_verb(kept, kept_count, "down"))) continue;
        if (!probe_verb_allowed(game, id, &state, probe_verbs[i].verb)) continue;
        Action action = {probe_verbs[i].label, probe_verbs[i].verb, NULL, DO_OBJECT};
        char noun[Z_NAME_MAX], reply[ACTION_REPLY_SIZE];
        if (!resolve_probe_candidate(action, id, noun_hint, noun, reply)) continue;
        kept[kept_count] = action;
        action_reply_normalized(reply, norm[kept_count], sizeof(norm[0]));
        action_reply_signature(reply, &signatures[kept_count]);
        append_text(nouns[kept_count], sizeof(nouns[0]), noun);
        kept_count++;
    }
    // Collapse actions only when the full captured replies and observed story
    // state changes match, keeping the earliest. Replies that only mean "not
    // available here" are not results.
    unsigned out_count = 0;
    bool output_groupable[MAX_ACTIONS] = {false};
    for (unsigned i = 0; i < kept_count; i++) {
        bool groupable = signatures[i].valid && norm[i][0] &&
            !text_has(norm[i], "you can't see any") && !text_has(norm[i], "you're not at") &&
            !text_has(norm[i], "there is no ");
        bool duplicate = false;
        if (groupable) {
            for (unsigned j = 0; j < out_count; j++) {
                if (!output_groupable[j] || !action_reply_signatures_match(signatures[j], signatures[i])) continue;
                if (!effect_attempted[j]) {
                    effect_attempted[j] = true;
                    effect_valid[j] = action_effect_signature(kept[j], nouns[j], signatures[j], &effects[j]);
                }
                if (!effect_attempted[i]) {
                    effect_attempted[i] = true;
                    effect_valid[i] = action_effect_signature(kept[i], nouns[i], signatures[i], &effects[i]);
                }
                if (effect_valid[j] && effect_valid[i] &&
                    effects[j].first == effects[i].first && effects[j].second == effects[i].second) {
                    duplicate = true;
                    break;
                }
            }
        }
        if (duplicate) continue;
        if (out_count != i) {
            kept[out_count] = kept[i];
            norm[out_count][0] = 0;
            if (groupable) append_text(norm[out_count], sizeof(norm[0]), norm[i]);
            signatures[out_count] = signatures[i];
            nouns[out_count][0] = 0;
            append_text(nouns[out_count], sizeof(nouns[0]), nouns[i]);
            effects[out_count] = effects[i];
            effect_attempted[out_count] = effect_attempted[i];
            effect_valid[out_count] = effect_valid[i];
        }
        output_groupable[out_count] = groupable;
        if (!groupable) {
            norm[out_count][0] = 0;
            signatures[out_count] = (ActionReplySignature){0};
            nouns[out_count][0] = 0;
            effects[out_count] = (ZProbeEffect){0};
            effect_attempted[out_count] = false;
            effect_valid[out_count] = false;
        }
        out_count++;
    }
    // Keep the list within the menu budget; the last slot stays for "More actions...".
    unsigned budget = has_menu ? MAX_ACTIONS - 2 : MAX_ACTIONS - 1;
    while (out_count > budget && out_count > 0) out_count--;
    if (has_menu) kept[out_count++] = (Action)MENU("More actions...", SHOW_CATEGORIES);
    for (unsigned i = 0; i < out_count; i++) list[i] = kept[i];
    action_menu_probe_budget = old_probe_budget;
    return out_count;
}

unsigned story_filter_defaults(unsigned id, Action list[MAX_ACTIONS], unsigned count) {
    bool carried = list_has_verb(list, count, "drop");
    return story_filter_defaults_internal(3, id, carried, list, count, false, UINT32_MAX,
                                          ACTION_REFERENCE_PROBE_BUDGET);
}

unsigned story_filter_defaults_masked(unsigned game, unsigned id,
                                      bool carried, Action list[MAX_ACTIONS], unsigned count) {
    uint32_t verbs = game < 3 ? find_action_candidate_mask(game, id, carried) : UINT32_MAX;
    return story_filter_defaults_internal(game, id, carried, list, count, true, verbs,
                                          STORY_MENU_PROBE_BUDGET);
}

// ---------------------------------------------------------------------------
// Incremental primary-menu filter
// ---------------------------------------------------------------------------
// The static list is shown immediately. Availability is then checked one
// candidate at a time, and the effect-based duplicate collapse runs one effect
// probe at a time, so the UI renders and accepts input between probes. The
// working set lives in IWRAM; see MENU_FILTER_SECTION.

enum { MENU_FILTER_PROBES = 32, MENU_FILTER_CANDIDATES = MAX_ACTIONS + MENU_FILTER_PROBES };
_Static_assert(sizeof(probe_verbs) / sizeof(probe_verbs[0]) == MENU_FILTER_PROBES,
               "update MENU_FILTER_PROBES when adding probe verbs");

// Resumable availability probe for one menu candidate. The z-machine probe is
// sliced across frames; this struct records where noun resolution left off.
enum { CAND_START, CAND_ISSUE, CAND_FINISH };
enum { RES_NONE, RES_HINT, RES_FULL, RES_SHORT_WORD, RES_SHORT_PAIR, RES_LITERAL };

typedef struct {
    bool active, probe_in_flight, action_done, normalize, normalize_started, hash_pending, restore_pending;
    bool answered, resolved, accepted;
    bool examine, literal, probe_verb;
    unsigned stage, resolution, index;
    Action action;
    char full[Z_NAME_MAX], noun[Z_NAME_MAX], short_word[Z_NAME_MAX];
    char reply[ACTION_REPLY_SIZE];
    unsigned starts[MAX_NOUN_WORDS], lengths[MAX_NOUN_WORDS], words, word_index;
    ZProbeEffect effect;
} CandidateJob;

typedef struct {
    bool active, probe_in_flight, action_done, normalize_started, hash_pending, restore_pending;
    unsigned accepted;
    char line[COMMAND_SIZE], reply[ACTION_REPLY_SIZE];
    ActionReplySignature expected;
    ZProbeEffect effect;
} EffectJob;

typedef struct {
    bool active, done;
    bool carried, has_menu;
    unsigned chapter, id;
    ActionStateSnapshot state;
    unsigned static_count, probe_count, candidate_count, cursor;
    unsigned accepted_count, result_count;
    unsigned collapse_cursor, collapse_j, collapse_out;
    Action static_actions[MAX_ACTIONS];
    uint8_t probe_indices[MENU_FILTER_PROBES];
    uint8_t accepted_indices[MAX_ACTIONS];
    char accepted_nouns[MAX_ACTIONS][Z_NAME_MAX];
    ActionReplySignature accepted_sig[MAX_ACTIONS];
    bool accepted_groupable[MAX_ACTIONS];
    bool accepted_flags[MENU_FILTER_CANDIDATES];
    char noun_hint[Z_NAME_MAX];
    uint8_t out_accepted[MAX_ACTIONS];
    ZProbeEffect effects[MAX_ACTIONS];
    bool effect_valid[MAX_ACTIONS];
    bool attempted[MAX_ACTIONS], valid[MAX_ACTIONS];
    Action result[MAX_ACTIONS];
    CandidateJob job;
    EffectJob effect_job;
} MenuFilter;

// The filter state is large and hot; keep it in the GBA's internal RAM, away
// from the crowded EWRAM. The host tests have no such section.
#if defined(__arm__) && !defined(__aarch64__)
#define MENU_FILTER_SECTION __attribute__((section(".iwram")))
#else
#define MENU_FILTER_SECTION
#endif
static MenuFilter menu_filter MENU_FILTER_SECTION;

static Action filter_candidate(unsigned index) {
    if (index < menu_filter.static_count) return menu_filter.static_actions[index];
    unsigned probe = menu_filter.probe_indices[index - menu_filter.static_count];
    Action action = {probe_verbs[probe].label, probe_verbs[probe].verb, NULL, DO_OBJECT};
    return action;
}

static bool filter_accepted_verb(const char *verb) {
    for (unsigned i = 0; i < menu_filter.accepted_count; i++) {
        Action action = filter_candidate(menu_filter.accepted_indices[i]);
        if (action.verb && text_equal(action.verb, verb)) return true;
    }
    return false;
}

static bool filter_groupable(const char *reply, ActionReplySignature signature) {
    char norm[48];
    if (!signature.valid) return false;
    action_reply_normalized(reply, norm, sizeof(norm));
    return norm[0] && !text_has(norm, "you can't see any") &&
           !text_has(norm, "you're not at") && !text_has(norm, "there is no ");
}

// Does this reply duplicate an action already accepted in this menu? If so,
// the availability probe runs the normalization pass and captures the effect,
// so the duplicate collapse can compare hashes without replaying the command.
static bool filter_reply_needs_effect(const char *reply) {
    ActionReplySignature signature = {0};
    if (parser_rejected(reply) || !action_reply_signature(reply, &signature)) return false;
    if (!filter_groupable(reply, signature)) return false;
    for (unsigned i = 0; i < menu_filter.accepted_count; i++) {
        if (menu_filter.accepted_groupable[i] &&
            action_reply_signatures_match(menu_filter.accepted_sig[i], signature))
            return true;
    }
    return false;
}

static void filter_take(unsigned index, const char *noun, const char *reply) {
    unsigned limit = menu_filter.has_menu ? MAX_ACTIONS - 2 : MAX_ACTIONS - 1;
    if (menu_filter.accepted_count >= limit) return;
    ActionReplySignature signature = {0};
    bool groupable = false;
    if (reply && reply[0]) {
        action_reply_signature(reply, &signature);
        groupable = filter_groupable(reply, signature);
    }
    unsigned accepted = menu_filter.accepted_count;
    menu_filter.accepted_indices[accepted] = (uint8_t)index;
    if (noun) append_text(menu_filter.accepted_nouns[accepted], Z_NAME_MAX, noun);
    menu_filter.accepted_sig[accepted] = signature;
    menu_filter.accepted_groupable[accepted] = groupable;
    menu_filter.effect_valid[accepted] = action_probe_effect_valid;
    if (action_probe_effect_valid) menu_filter.effects[accepted] = action_probe_effect;
    menu_filter.accepted_count++;
    menu_filter.accepted_flags[index] = true;
}

static bool menu_probe_take_budget(void) {
    if (!action_menu_probe_limited) return true;
    if (!action_menu_probes_remaining) return false;
    action_menu_probes_remaining--;
    action_menu_probes_used_count++;
    return true;
}

static unsigned tokenize_noun(const char *text, unsigned starts[], unsigned lengths[]) {
    unsigned words = 0;
    for (unsigned i = 0; text[i] && words < MAX_NOUN_WORDS;) {
        while (text[i] == ' ') i++;
        if (!text[i]) break;
        starts[words] = i;
        while (text[i] && text[i] != ' ') i++;
        lengths[words] = i - starts[words];
        words++;
    }
    return words;
}

static bool candidate_finish(void) {
    CandidateJob *j = &menu_filter.job;
    if (j->examine && j->resolved && !menu_filter.noun_hint[0])
        append_text(menu_filter.noun_hint, sizeof(menu_filter.noun_hint), j->noun);
    if (j->accepted) filter_take(j->index, j->noun, j->resolved ? j->reply : NULL);
    j->active = false;
    j->stage = CAND_FINISH;
    return true;
}

static bool candidate_check(void) {
    CandidateJob *j = &menu_filter.job;
    j->accepted = true;
    if (j->probe_verb) {
        if (j->action.verb && text_equal(j->action.verb, "climb up") &&
            reply_says_climb_direction_fails(j->reply, true)) j->accepted = false;
        if (j->action.verb && text_equal(j->action.verb, "climb down") &&
            reply_says_climb_direction_fails(j->reply, false)) j->accepted = false;
        if (j->accepted && (reply_disallows_action(j->action, j->reply) ||
                            action_reply_is_no_effect(j->reply))) j->accepted = false;
    } else if (j->literal) {
        if (reply_disallows_action(j->action, j->reply)) j->accepted = false;
        if (j->accepted && is_vertical_route_action(j->action) &&
            reply_blocks_vertical_route(j->reply)) j->accepted = false;
    } else if (!j->examine) {
        if (reply_disallows_action(j->action, j->reply) ||
            action_reply_is_no_effect(j->reply)) j->accepted = false;
    }
    j->stage = CAND_FINISH;
    return candidate_finish();
}

static bool candidate_unresolved(void) {
    CandidateJob *j = &menu_filter.job;
    if (j->examine) {
        // Examine is always offered; without a resolved noun it keeps no reply.
        j->accepted = true;
        j->resolved = false;
        j->noun[0] = 0;
    } else j->accepted = false;
    j->stage = CAND_FINISH;
    return candidate_finish();
}

static bool candidate_short_init(const char *base) {
    CandidateJob *j = &menu_filter.job;
    if (base != j->full) {
        j->full[0] = 0;
        append_text(j->full, Z_NAME_MAX, base);
    }
    j->words = tokenize_noun(j->full, j->starts, j->lengths);
    if (j->words < 2) return candidate_unresolved();
    j->word_index = 0;
    j->resolution = RES_SHORT_WORD;
    copy_noun_words(j->full, 0, 1, j->starts, j->lengths, j->short_word);
    j->stage = CAND_ISSUE;
    return false;
}

static bool candidate_resolution_failed(void) {
    CandidateJob *j = &menu_filter.job;
    if (j->resolution == RES_HINT || j->resolution == RES_FULL) {
        if (reply_allows_noun_fallback(j->reply))
            return candidate_short_init(j->resolution == RES_HINT ? menu_filter.noun_hint : j->full);
        return candidate_unresolved();
    }
    if (j->resolution == RES_SHORT_WORD) {
        if (!reply_allows_noun_fallback(j->reply)) return candidate_unresolved();
        j->word_index++;
        if (j->word_index < j->words) {
            copy_noun_words(j->full, j->word_index, 1, j->starts, j->lengths, j->short_word);
            j->stage = CAND_ISSUE;
            return false;
        }
        if (j->words > 2) {
            copy_noun_words(j->full, j->words - 2, 2, j->starts, j->lengths, j->short_word);
            j->resolution = RES_SHORT_PAIR;
            j->stage = CAND_ISSUE;
            return false;
        }
        return candidate_unresolved();
    }
    if (j->resolution == RES_SHORT_PAIR) return candidate_unresolved();
    if (j->resolution == RES_LITERAL) {
        j->accepted = false;
        j->stage = CAND_FINISH;
        return candidate_finish();
    }
    return candidate_unresolved();
}

static bool candidate_handle_result(bool answered) {
    CandidateJob *j = &menu_filter.job;
    j->answered = answered;
    if (!answered) return candidate_resolution_failed();
    bool noun_supplied = j->resolution != RES_LITERAL;
    if (parser_rejected(j->reply) ||
        (noun_supplied && reply_has_no_matching_target(j->reply)))
        return candidate_resolution_failed();
    if (j->resolution == RES_LITERAL) {
        j->resolved = true;
    } else {
        const char *used = j->resolution == RES_HINT ? menu_filter.noun_hint :
                           j->resolution == RES_FULL ? j->full : j->short_word;
        j->noun[0] = 0;
        append_text(j->noun, Z_NAME_MAX, used);
        j->resolved = true;
    }
    return candidate_check();
}

static bool candidate_issue(void) {
    CandidateJob *j = &menu_filter.job;
    const char *noun = NULL;
    switch (j->resolution) {
        case RES_HINT: noun = menu_filter.noun_hint; break;
        case RES_FULL: noun = j->full; break;
        case RES_SHORT_WORD:
        case RES_SHORT_PAIR: noun = j->short_word; break;
        case RES_LITERAL: noun = NULL; break;
        default: return candidate_unresolved();
    }
    char line[COMMAND_SIZE];
    if (!raw_action_command(j->action, noun, NULL, line)) return candidate_unresolved();
    if (!menu_probe_take_budget()) return candidate_unresolved();
    ZProbeStep step = z_probe_menu_begin(line, j->reply, sizeof(j->reply),
                                        action_menu_probe_budget, STORY_MENU_PROBE_CHUNK);
    j->probe_in_flight = true;
    j->action_done = false;
    j->normalize = false;
    j->normalize_started = false;
    action_probe_effect_valid = false;
    if (step == Z_PROBE_BUSY) return false;
    if (step == Z_PROBE_FAILED) {
        bool answered = z_probe_menu_complete(NULL);
        j->probe_in_flight = false;
        return candidate_handle_result(answered);
    }
    // The action finished inside this chunk; decide whether it duplicates an
    // accepted reply and, if so, start the normalization on the next step.
    j->action_done = true;
    j->normalize = filter_reply_needs_effect(j->reply);
    if (!j->normalize) {
        bool answered = z_probe_menu_complete(NULL);
        j->probe_in_flight = false;
        return candidate_handle_result(answered);
    }
    return false;
}

static void candidate_start(unsigned index) {
    CandidateJob *j = &menu_filter.job;
    *j = (CandidateJob){0};
    j->active = true;
    j->index = index;
    j->stage = CAND_START;
    action_probe_effect_valid = false;
    if (index < menu_filter.static_count) {
        j->action = menu_filter.static_actions[index];
        j->examine = j->action.verb && text_equal(j->action.verb, "examine");
        j->literal = j->action.kind == DO_LITERAL;
        if (j->action.verb && !probe_verb_allowed(menu_filter.chapter, menu_filter.id,
                                                   &menu_filter.state, j->action.verb)) {
            j->accepted = false;
            j->stage = CAND_FINISH;
            return;
        }
        if (action_state_prevalidated(menu_filter.chapter, menu_filter.id,
                                      &menu_filter.state, j->action)) {
            j->accepted = true;
            j->stage = CAND_FINISH;
            return;
        }
        if (j->examine) {
            story_parser_noun(menu_filter.id, j->full);
            if (!j->full[0]) {
                j->accepted = true;
                j->resolved = false;
                j->stage = CAND_FINISH;
            } else j->resolution = RES_FULL;
        } else if (j->action.kind == DO_OBJECT) {
            if (menu_filter.noun_hint[0]) j->resolution = RES_HINT;
            else {
                story_parser_noun(menu_filter.id, j->full);
                if (!j->full[0]) {
                    j->accepted = false;
                    j->stage = CAND_FINISH;
                } else j->resolution = RES_FULL;
            }
        } else if (j->literal) {
            j->resolution = RES_LITERAL;
        } else {
            j->accepted = true;
            j->stage = CAND_FINISH;
        }
    } else {
        unsigned probe = menu_filter.probe_indices[index - menu_filter.static_count];
        j->action = (Action){probe_verbs[probe].label, probe_verbs[probe].verb, NULL, DO_OBJECT};
        j->probe_verb = true;
        if (menu_filter.noun_hint[0]) j->resolution = RES_HINT;
        else {
            story_parser_noun(menu_filter.id, j->full);
            if (!j->full[0]) {
                j->accepted = false;
                j->stage = CAND_FINISH;
            } else j->resolution = RES_FULL;
        }
    }
    if (j->stage != CAND_FINISH) j->stage = CAND_ISSUE;
}

static bool filter_probe_skipped(unsigned index) {
    const char *verb = probe_verbs[menu_filter.probe_indices[index - menu_filter.static_count]].verb;
    if (filter_accepted_verb(verb) ||
        (text_equal(verb, "light") && filter_accepted_verb("turn on")) ||
        (text_equal(verb, "extinguish") && filter_accepted_verb("turn off"))) return true;
    if (text_equal(verb, "climb") &&
        (filter_accepted_verb("climb up") || filter_accepted_verb("climb down") ||
         filter_accepted_verb("up") || filter_accepted_verb("down"))) return true;
    return !probe_verb_allowed(menu_filter.chapter, menu_filter.id, &menu_filter.state, verb);
}

static bool filter_job_step(void) {
    CandidateJob *j = &menu_filter.job;
    if (!j->active) return true;
    if (j->hash_pending) {
        z_probe_menu_hash(&j->effect);
        j->hash_pending = false;
        j->restore_pending = true;
        return false;
    }
    if (j->restore_pending) {
        bool answered = z_probe_menu_complete(&j->effect);
        j->restore_pending = false;
        j->probe_in_flight = false;
        action_probe_effect = j->effect;
        action_probe_effect_valid = answered && j->effect.valid;
        return candidate_handle_result(answered);
    }
    if (j->stage == CAND_FINISH) return candidate_finish();
    if (!j->probe_in_flight) {
        if (j->stage == CAND_ISSUE) return candidate_issue();
        return candidate_finish();
    }
    if (!j->action_done) {
        ZProbeStep step = z_probe_menu_continue(STORY_MENU_PROBE_CHUNK);
        if (step == Z_PROBE_BUSY) return false;
        if (step == Z_PROBE_FAILED) {
            bool answered = z_probe_menu_complete(NULL);
            j->probe_in_flight = false;
            return candidate_handle_result(answered);
        }
        j->action_done = true;
        j->normalize = filter_reply_needs_effect(j->reply);
        if (!j->normalize) {
            bool answered = z_probe_menu_complete(NULL);
            j->probe_in_flight = false;
            return candidate_handle_result(answered);
        }
        return false;
    }
    if (!j->normalize_started) {
        ZProbeStep step = z_probe_menu_normalize(STORY_MENU_PROBE_CHUNK);
        j->normalize_started = true;
        if (step == Z_PROBE_BUSY) return false;
        j->hash_pending = true;
        return false;
    }
    ZProbeStep step = z_probe_menu_continue(STORY_MENU_PROBE_CHUNK);
    if (step == Z_PROBE_BUSY) return false;
    j->hash_pending = true;
    return false;
}

static bool effect_replay_start(unsigned accepted) {
    EffectJob *e = &menu_filter.effect_job;
    if (e->active) return true;
    if (!menu_filter.accepted_sig[accepted].valid) return false;
    Action action = filter_candidate(menu_filter.accepted_indices[accepted]);
    char line[COMMAND_SIZE];
    if (!raw_action_command(action, menu_filter.accepted_nouns[accepted], NULL, line)) return false;
    if (action_menu_probe_limited) {
        if (!action_menu_effect_probes_remaining) return false;
        action_menu_effect_probes_remaining--;
        action_menu_effect_probes_used_count++;
    }
    *e = (EffectJob){0};
    append_text(e->line, sizeof(e->line), line);
    e->accepted = accepted;
    e->expected = menu_filter.accepted_sig[accepted];
    e->active = true;
    ZProbeStep step = z_probe_menu_begin(e->line, e->reply, sizeof(e->reply),
                                        action_menu_probe_budget, STORY_MENU_PROBE_CHUNK);
    e->probe_in_flight = true;
    if (step == Z_PROBE_FAILED) {
        z_probe_menu_complete(NULL);
        menu_filter.attempted[accepted] = true;
        menu_filter.valid[accepted] = false;
        e->active = false;
        return true;
    }
    e->action_done = step == Z_PROBE_DONE;
    return true;
}

static bool effect_replay_finish(void) {
    EffectJob *e = &menu_filter.effect_job;
    bool answered = z_probe_menu_complete(&e->effect);
    ActionReplySignature observed = {0};
    bool valid = answered && e->effect.valid &&
                 action_reply_signature(e->reply, &observed) &&
                 action_reply_signatures_match(e->expected, observed);
    menu_filter.attempted[e->accepted] = true;
    menu_filter.valid[e->accepted] = valid;
    if (valid) menu_filter.effects[e->accepted] = e->effect;
    e->active = false;
    return true;
}

// One frame of the effect replay. Returns false while the probe is still
// running; true when the replay (or its failure) has been recorded.
static bool effect_replay_step(void) {
    EffectJob *e = &menu_filter.effect_job;
    if (!e->active) return true;
    if (e->hash_pending) {
        z_probe_menu_hash(&e->effect);
        e->hash_pending = false;
        e->restore_pending = true;
        return false;
    }
    if (e->restore_pending) {
        e->restore_pending = false;
        return effect_replay_finish();
    }
    if (!e->action_done) {
        ZProbeStep step = z_probe_menu_continue(STORY_MENU_PROBE_CHUNK);
        if (step == Z_PROBE_BUSY) return false;
        if (step == Z_PROBE_FAILED) {
            z_probe_menu_complete(NULL);
            menu_filter.attempted[e->accepted] = true;
            menu_filter.valid[e->accepted] = false;
            e->active = false;
            return true;
        }
        e->action_done = true;
    }
    if (!e->normalize_started) {
        ZProbeStep step = z_probe_menu_normalize(STORY_MENU_PROBE_CHUNK);
        e->normalize_started = true;
        if (step == Z_PROBE_BUSY) return false;
    } else {
        ZProbeStep step = z_probe_menu_continue(STORY_MENU_PROBE_CHUNK);
        if (step == Z_PROBE_BUSY) return false;
    }
    e->hash_pending = true;
    return false;
}

// One step of the reference collapse: compare the current accepted action with
// the actions already kept. If two groupable replies match, use the effect hash
// captured by the availability probe; replay only when no hash was captured.
// The replay itself is sliced across frames.
static bool filter_collapse_step(void) {
    if (menu_filter.effect_job.active) {
        effect_replay_step();
        return false;
    }
    while (menu_filter.collapse_cursor < menu_filter.accepted_count) {
        unsigned a = menu_filter.collapse_cursor;
        ActionReplySignature signature = menu_filter.accepted_sig[a];
        bool groupable = menu_filter.accepted_groupable[a];
        bool duplicate = false;
        while (groupable && menu_filter.collapse_j < menu_filter.collapse_out) {
            unsigned j = menu_filter.collapse_j;
            unsigned other = menu_filter.out_accepted[j];
            if (!menu_filter.accepted_groupable[other] ||
                !action_reply_signatures_match(menu_filter.accepted_sig[other], signature)) {
                menu_filter.collapse_j++;
                continue;
            }
            if (!menu_filter.effect_valid[a] && !menu_filter.attempted[a]) {
                if (!effect_replay_start(a)) {
                    menu_filter.attempted[a] = true;
                    menu_filter.valid[a] = false;
                } else return false;
            }
            if (!menu_filter.effect_valid[other] && !menu_filter.attempted[other]) {
                if (!effect_replay_start(other)) {
                    menu_filter.attempted[other] = true;
                    menu_filter.valid[other] = false;
                } else return false;
            }
            bool a_effect = menu_filter.effect_valid[a] || menu_filter.valid[a];
            bool other_effect = menu_filter.effect_valid[other] || menu_filter.valid[other];
            if (a_effect && other_effect &&
                menu_filter.effects[a].first == menu_filter.effects[other].first &&
                menu_filter.effects[a].second == menu_filter.effects[other].second) {
                duplicate = true;
                break;
            }
            menu_filter.collapse_j++;
        }
        if (!duplicate)
            menu_filter.out_accepted[menu_filter.collapse_out++] = (uint8_t)a;
        menu_filter.collapse_cursor++;
        menu_filter.collapse_j = 0;
    }

    menu_filter.result_count = 0;
    for (unsigned i = 0; i < menu_filter.collapse_out; i++)
        menu_filter.result[menu_filter.result_count++] =
            filter_candidate(menu_filter.accepted_indices[menu_filter.out_accepted[i]]);
    if (menu_filter.has_menu && menu_filter.result_count < MAX_ACTIONS)
        menu_filter.result[menu_filter.result_count++] = (Action)MENU("More actions...", SHOW_CATEGORIES);
    return true;
}

bool story_menu_filter_begin(unsigned chapter, unsigned id, bool carried,
                             Action list[MAX_ACTIONS], unsigned count) {
    if (menu_filter.active) story_menu_filter_cancel();
    z_probe_menu_abort();
    menu_filter = (MenuFilter){0};
    menu_filter.chapter = chapter;
    menu_filter.id = id;
    menu_filter.carried = carried;
    menu_filter.state = capture_action_state(chapter, id, carried);

    menu_filter.has_menu = count && list[count - 1].kind == SHOW_CATEGORIES;
    if (menu_filter.has_menu) count--;
    if (count > MAX_ACTIONS) count = MAX_ACTIONS;
    for (unsigned i = 0; i < count; i++) menu_filter.static_actions[i] = list[i];
    menu_filter.static_count = count;

    uint32_t mask = chapter < 3 ? find_action_candidate_mask(chapter, id, carried) : UINT32_MAX;
    for (unsigned i = 0; i < MENU_FILTER_PROBES; i++) {
        if (!(mask & (1u << i))) continue;
        if (!probe_verb_allowed(chapter, id, &menu_filter.state, probe_verbs[i].verb)) continue;
        menu_filter.probe_indices[menu_filter.probe_count++] = (uint8_t)i;
    }
    menu_filter.candidate_count = menu_filter.static_count + menu_filter.probe_count;
    menu_filter.active = true;
    story_menu_probe_begin();
    story_menu_probe_set_budget(STORY_MENU_PROBE_BUDGET);
#ifdef PROFILE
    z_profile_tokenize_ticks = 0;
    z_profile_hash_ticks = 0;
    z_profile_restore_ticks = 0;
#endif
#ifdef PROFILE_TRACE
    z_profile_slow_pc = z_profile_slow_ticks = 0;
    z_profile_slow_opcode = 0;
#endif
    return true;
}

bool story_menu_filter_active(void) { return menu_filter.active; }

bool story_menu_filter_step(void) {
    if (!menu_filter.active) return true;
    if (menu_filter.cursor < menu_filter.candidate_count || menu_filter.job.active) {
        CandidateJob *j = &menu_filter.job;
        if (!j->active) {
            unsigned index = menu_filter.cursor;
            if (index >= menu_filter.static_count && filter_probe_skipped(index)) {
                menu_filter.cursor++;
                return false;
            }
            candidate_start(index);
            menu_filter.cursor++;
        }
        if (filter_job_step()) j->active = false;
        return false;
    }
    if (!filter_collapse_step()) return false;
    menu_filter.done = true;
    menu_filter.active = false;
    story_menu_probe_end();
    return true;
}

unsigned story_menu_filter_display(Action list[MAX_ACTIONS], unsigned capacity) {
    if (!capacity) return 0;
    if (menu_filter.done) {
        unsigned n = menu_filter.result_count < capacity ? menu_filter.result_count : capacity;
        for (unsigned i = 0; i < n; i++) list[i] = menu_filter.result[i];
        return n;
    }
    unsigned n = 0;
    unsigned limit = menu_filter.has_menu && capacity ? capacity - 1 : capacity;
    // Unprocessed static candidates stay visible until their probe returns.
    for (unsigned i = 0; i < menu_filter.static_count && n < limit; i++) {
        Action action = menu_filter.static_actions[i];
        if (action.verb && !probe_verb_allowed(menu_filter.chapter, menu_filter.id,
                                               &menu_filter.state, action.verb))
            continue;
        bool in_flight = menu_filter.job.active && menu_filter.job.index == i;
        if (menu_filter.accepted_flags[i] || in_flight || i >= menu_filter.cursor)
            list[n++] = action;
    }
    for (unsigned i = 0; i < menu_filter.probe_count && n < limit; i++) {
        unsigned index = menu_filter.static_count + i;
        if (index < menu_filter.cursor && menu_filter.accepted_flags[index])
            list[n++] = filter_candidate(index);
    }
    if (menu_filter.has_menu && n < capacity)
        list[n++] = (Action)MENU("More actions...", SHOW_CATEGORIES);
    return n;
}

void story_menu_filter_cancel(void) {
    if (!menu_filter.active) return;
    z_probe_menu_abort();
    story_menu_probe_end();
    menu_filter.active = false;
    menu_filter.done = false;
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

bool speech_command(WordMode mode, const char *phrase, char line[COMMAND_SIZE]) {
    static const char *const prefix[] = {
        [WORDS_SAY] = "say \"", [WORDS_ANSWER] = "answer \"",
        [WORDS_COMMAND] = "", [WORDS_GREET] = "hello "
    };
    line[0] = 0;
    return mode < sizeof(prefix) / sizeof(prefix[0]) && *phrase &&
           append_text(line, COMMAND_SIZE, prefix[mode]) && append_text(line, COMMAND_SIZE, phrase) &&
           ((mode != WORDS_SAY && mode != WORDS_ANSWER) || append_text(line, COMMAND_SIZE, "\""));
}

void speech_word(unsigned index, char word[Z_WORD_MAX]) {
    z_dictionary_word(index, word);
    if (text_equal(word, "ulysse")) append_text(word, Z_WORD_MAX, "s");
    else if (text_equal(word, "odysse")) append_text(word, Z_WORD_MAX, "us");
}
