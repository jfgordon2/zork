#include "interaction.h"
#include "save.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>




static unsigned chapter, seed;
static bool strict;
static ZResume base;
static uint8_t story[96 * 1024], dynamic[Z_DYNAMIC_MAX], stack[Z_STACK_WORDS * 2];
static char out[16384], chosen[COMMAND_SIZE];
static const char *expected;
static Targets room, bag;
static bool attempted_exact, found;
static int best_rank;

static void output(const char *s, size_t n) {
    size_t left = sizeof(out) - strlen(out) - 1;
    strncat(out, s, left < n ? left : n);
}
static void fatal(const char *err, unsigned pc, unsigned opcode) {
    fprintf(stderr, "%s at %x %x from %s\n", err, pc, opcode, expected); exit(1);
}
static uint32_t entropy(void) { return seed; }
static const ZPlatform io = {output, fatal, entropy, NULL, NULL};

typedef struct {
    char response[16384];
    unsigned end, room, score, moves;
    unsigned objects;
    uint32_t random;
} Result;

static Result baseline;
static unsigned object_hash(void) {
    unsigned hash = 2166136261u;
    unsigned table = z_dynamic()[10] * 256 + z_dynamic()[11] + 62;
    for (unsigned i = 0; i < 9 * z_object_count(); i++) hash = (hash ^ z_dynamic()[table + i]) * 16777619u;
    return hash;
}
static void play(const char *line, Result *result) {
    z_load(&base, dynamic, stack);
    out[0] = 0;
    assert(z_submit(line));
    ZResult next = z_run(1000000);
    assert(next != Z_BUSY);
    strcpy(result->response, out);
    result->end = next == Z_ENDED;
    result->room = z_global(0);
    result->score = z_global(1);
    result->moves = z_global(2);
    result->objects = object_hash();
    result->random = z_resume().random;
}

static int direction(const char *line) {
    for (unsigned i = 0; i < 12; i++) {
        if (text_equal(line,path_names[i]) || text_equal(line,path_commands[i])) return (int)i;
    }
    return -1;
}

static const char *verb_group(const char *line) {
    if (text_has(line, ", ")) return "order";
    static char first[2][32];
    static unsigned slot;
    slot ^= 1;
    sscanf(line,"%31s",first[slot]);
    const char *verb = first[slot];
    if (text_equal(verb,"get") && text_has(line,"out")) return "leave";
    if (text_equal(verb,"out") || text_equal(verb,"exit")) return "leave";
    if (text_equal(verb,"get") || text_equal(verb,"take")) return "take";
    if (text_equal(verb,"hit") || text_equal(verb,"kill") || text_equal(verb,"attack")) return "attack";
    if (text_equal(verb,"press") || text_equal(verb,"push")) return "push";
    if (text_equal(verb,"lift") || text_equal(verb,"raise")) return "raise";
    return verb;
}

static int word_rank(const char *a, const char *b) {
    int score = 0;
    char first[COMMAND_SIZE], next[COMMAND_SIZE];
    strcpy(first,a); strcpy(next,b);
    char *remainder = NULL;
    for (const char *word = strtok_r(first, " ,.-", &remainder); word; word = strtok_r(NULL, " ,.-", &remainder)) {
        const char *ptr = next;
        size_t size = strlen(word);
        while (*ptr) {
            size_t len = strcspn(ptr, " ,.-");
            if (size == len && !strncasecmp(word,ptr,len)) {
                score += size;
                break;
            }
            ptr += len;
            ptr += strspn(ptr, " ,.-");
        }
    }
    return score;
}

static void consider(const char *command) {
    if (found || strlen(command) > z_input_limit()) return;
    if (text_equal(command, expected)) {
        strcpy(chosen, command);
        attempted_exact = found = true;
        return;
    }
    int dir = direction(expected), selected = direction(command);
    if (dir >= 0 && selected != dir) return;
    if (dir < 0 && !text_equal(verb_group(command), verb_group(expected))) return;
    int rank = word_rank(expected,command);
    if (rank <= best_rank) return;
    static Result result;
    play(command, &result);
    z_load(&base, dynamic, stack);
    if (result.room == baseline.room && result.score == baseline.score && result.moves == baseline.moves &&
        result.end == baseline.end && result.objects == baseline.objects && result.random == baseline.random &&
        !strcmp(result.response, baseline.response)) {
        strcpy(chosen, command);
        best_rank = rank;
    }
}

static void action(Action a, unsigned object, const char *value) {
    char command[COMMAND_SIZE];
    if (action_command(a, object, value, command)) consider(command);
}

static void targets(Action a, unsigned object) {
    char name[Z_NAME_MAX];
    if (a.kind == PICK_SELF) action(a, object, "me");
    const Targets *lists[] = {&room, &bag};
    for (unsigned l = 0; l < 2 && !found; l++) {
        for (unsigned i = 0; i < lists[l]->count && !found; i++) {
            unsigned t = lists[l]->id[i];
            if (t == object) continue;
            story_noun(t, name);
            action(a, object, name);
        }
    }
}

static bool dictionary_has(const char *phrase) {
    char copy[64];
    if (strlen(phrase) >= sizeof(copy)) return false;
    strcpy(copy, phrase);
    unsigned count = 0;
    for (const char *part = strtok(copy," "); part; part = strtok(NULL," ")) {
        if (++count > WORDS_PER_PHRASE || *part < 'a' || *part > 'z') return false;
        bool present = false;
        for (unsigned i = 0; i < z_dictionary_size(); i++) {
            char word[Z_WORD_MAX];
            speech_word(i, word);
            if (text_equal(word,part)) { present = true; break; }
        }
        if (!present) return false;
    }
    return count != 0;
}

static void speech(void) {
    char phrase[64], command[COMMAND_SIZE];
    const char *quote = strchr(expected, '"');
    unsigned mode = 2;
    if (quote && quote[1] && expected[strlen(expected) - 1] == '"') {
        size_t n = strlen(quote + 1) - 1;
        if (n >= sizeof(phrase)) return;
        memcpy(phrase,quote + 1,n); phrase[n] = 0;
        if (!strncmp(expected,"say ",4)) mode = 0;
        else if (!strncmp(expected,"answer ",7)) mode = 1;
        else return;
    } else {
        const char *words = expected;
        if (!strncmp(expected,"hello ",6)) { mode = 3; words += 6; }
        else if (strchr(expected,' ')) return;
        if (strlen(words) >= sizeof(phrase)) return;
        strcpy(phrase,words);
    }
    if (dictionary_has(phrase) && speech_command(mode, phrase, command)) consider(command);
}

static void enumerate(void) {
    for (unsigned i = 0; i < 12; i++) consider(path_commands[i]);
    const char *fixed[] = {"listen", "wait", "look", "score", "take all", "drop all", "pray", "launch", "dive", "yes", "no"};
    for (unsigned i = 0; i < sizeof(fixed)/sizeof(*fixed); i++) consider(fixed[i]);
    // Non-turn commands used by the transcripts for inspection or to close the story.
    if (!strcmp(expected,"inventory") || !strcmp(expected,"quit")) consider(expected);
    if (!found) speech();
    const Targets *lists[] = {&room,&bag};
    for (unsigned l = 0; l < 2 && !found; l++) {
        for (unsigned i = 0; i < lists[l]->count && !found; i++) {
            unsigned object = lists[l]->id[i];
            bool can_order = false;
            for (ActionGroup group = PRIMARY; group <= ORDERS && !found; group++) {
                if (group == ORDERS && !can_order) continue;
                Action row[MAX_ACTIONS];
                unsigned count = story_actions(chapter, object, z_parent(object) == chapters[chapter].player, group, row);
                assert(count <= MAX_ACTIONS);
                for (unsigned j = 0; j < count && !found; j++) {
                    Action a = row[j];
                    switch (a.kind) {
                        case SHOW_ORDERS: can_order = true; break;
                        case DO_OBJECT:
                        case DO_LITERAL: action(a,object,NULL); break;
                        case PICK_TARGET:
                        case PICK_SELF: targets(a,object); break;
                        case PICK_DIRECTION:
                            for (unsigned d = 0; d < 12; d++) action(a,object,path_names[d]);
                            break;
                        case PICK_DIGIT:
                        case PICK_YEAR: {
                            const char *value = strrchr(expected,' ');
                            if (!value || !isdigit(value[1])) break;
                            unsigned number = atoi(value + 1);
                            if (a.kind == PICK_DIGIT ? (number < 1 || number > 8) : number > 999) break;
                            action(a,object,value+1);
                            break;
                        }
                        default: break;
                    }
                }
            }
        }
    }
}

static void scope(const Targets *targets) {
    for (unsigned i = 0; i < targets->count; i++) {
        char name[Z_NAME_MAX];
        unsigned id = targets->id[i];
        story_noun(id,name);
        printf(" %u=%s",id,name);
    }
    puts("");
}

int main(int argc, char **argv) {
    assert(argc >= 3);
    chapter = atoi(argv[1]) - 1;
    strict = argc == 4;
    const unsigned seeds[] = {7,4372,15};
    seed = seeds[chapter];
    char path[64], row[256];
    snprintf(path,sizeof(path),"stories/zork%u.z3",chapter+1);
    FILE *file = fopen(path,"rb");
    assert(file);
    size_t size = fread(story,1,sizeof(story),file);
    fclose(file);
    z_start(story,size,&io);
    assert(z_run(1000000) == Z_PROMPT);
    FILE *moves = fopen(argv[2],"r");
    assert(moves);
    unsigned coin_parent = z_parent(246);
    unsigned covered = 0, missed = 0, changed = 0, replaced = 0, skipped = 0;
    while (fgets(row,sizeof(row),moves)) {
        if (strncmp(row,">>>> ",5)) continue;
        char *end = strstr(row," [room ");
        assert(end);
        *end = 0;
        expected = row + 5;
        unsigned original_room, original_score;
        sscanf(end+1,"[room #%u score %u]",&original_room,&original_score);
        if (z_global(0) != original_room || z_global(1) != original_score) {
            printf("DRIFT at %s: room %u wanted %u, score %u wanted %u\n",
                expected, z_global(0), original_room, z_global(1), original_score);
        }
        if (text_equal(expected,"inventory") || text_equal(expected,"quit")) continue;
        base = z_resume();
        memcpy(dynamic,z_dynamic(),z_dynamic_size());
        memcpy(stack,z_stack_bytes(),base.sp * 2);
        story_targets(chapter,&room,&bag);
        play(expected,&baseline);
        z_load(&base,dynamic,stack);
        attempted_exact = found = false;
        best_rank = -1;
        enumerate();
        found |= best_rank >= 0;
        if (!found && (text_has(baseline.response, "You can't see any") || text_has(baseline.response, "You don't have") ||
            text_has(baseline.response, "Which key do you mean") || text_has(baseline.response, "sentence isn't one I recognize"))) {
            if (baseline.moves == z_global(2) && baseline.objects == object_hash() && baseline.random == base.random
                && baseline.room == z_global(0) && baseline.score == z_global(1)) {
                skipped++;
                printf("SKIP %s\n",expected);
                continue;
            }
            static const char *const passing_turn[] = {"listen","look","wait","score"};
            for (unsigned i = 0; i < sizeof(passing_turn) / sizeof(passing_turn[0]); i++) {
                Result turn;
                play(passing_turn[i], &turn);
                z_load(&base,dynamic,stack);
                if (turn.moves == baseline.moves && turn.objects == baseline.objects && turn.random == baseline.random
                    && turn.room == baseline.room && turn.score == baseline.score) {
                    replaced++;
                    printf("SUB %s instead of %s\n",passing_turn[i],expected);
                    strcpy(chosen,passing_turn[i]);
                    found = attempted_exact = true;
                    break;
                }
            }
        }
        if (!found) {
            if (chapter == 1 && z_global(0) == 125) printf("secret attr9=%d dark=%d\n",z_attr(151,9),story_dark(1));
            missed++;
            printf("MISS [%u] %s\nroom:",z_global(0),expected);
            scope(&room);
            printf("bag:");
            scope(&bag);
            printf("response: %.120s\n",baseline.response);
            if (strict) break;
        } else {
            covered++;
            if (!attempted_exact) {
                changed++;
                printf("ALIAS %s -> %s\n",expected,chosen);
            }
        }
        static Result final;
        play(found ? chosen : expected, &final);
        if (chapter == 1 && (coin_parent != z_parent(246) || text_has(expected, "coin"))) {
            printf("COIN %s => %s (parent %u -> %u) %.100s\n", expected, chosen, coin_parent, z_parent(246), final.response);
            coin_parent = z_parent(246);
        }
        if (final.end) break;
    }
    fclose(moves);
    printf("COVERAGE %u exact, %u equivalent, %u missing, score %u in room %u\n",
           covered - changed,changed,missed,z_global(1),z_global(0));
    printf("Failed test actions trimmed: %u omitted, %u waits\n", skipped,replaced);
    const unsigned score[] = {350,400,7}, room_id[] = {244,230,46};
    return strict && (missed || z_global(1) != score[chapter] || z_global(0) != room_id[chapter]);
}
