// Build cartridge save checkpoints for gallery rooms that no traced route stops in.
//
// For each chapter, replays the deterministic winning route from tests/fixtures (the same
// seeds as tests/transcript_replay_test.c). After every route step it snapshots the game, then
// for each still-missing room searches the story's own static exits for a path from the current
// room, walks it with ordinary direction commands, and accepts the result only if the live
// story reports that room, lit, at a prompt. The accepted state is written as a normal 32 KB
// cartridge SRAM image that the mGBA capture queue can Continue from.
//
// Build and run from the repository root:
//   cc -O2 -std=c11 -Isrc qa/mgba-room-gallery/route_saves.c src/zmachine.c src/save.c \
//      src/trail.c src/visual_state.c -o build/route-saves
//   build/route-saves CHAPTER SPEC [SPEC...]   (chapter 1-3; SPEC = ROOM or ROOM=step;step;...)
// Output: qa/mgba-room-gallery/checkpoints/zork-<n>-room-<id>.sav and .txt (the command log).
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "save.h"
#include "zmachine.h"

enum { MAX_ROUTE = 1024, MAX_PATH = 64, ROOMS = 256 };
static const uint8_t direction_props[12] = {31, 30, 28, 29, 27, 25, 24, 26, 23, 22, 21, 20};
static const char *const direction_words[12] = {"n", "e", "s", "w", "ne", "se", "sw", "nw", "up", "down", "in", "out"};
static const unsigned lit_globals[3] = {51, 52, 62};
static const unsigned seeds[3] = {7, 4372, 15};

static uint8_t story[128 * 1024], snapshot[SRAM_SIZE], output[SRAM_SIZE];
static char route[MAX_ROUTE][256];
static unsigned route_count, chapter, seed;
static uint8_t visited[VISITED_BYTES];
static Trail trail;
static char last_text[4096];
static size_t last_size;

static void text(const char *s, size_t n) {
    for (size_t i = 0; i < n && last_size + 1 < sizeof(last_text); i++) last_text[last_size++] = s[i];
    last_text[last_size] = 0;
}
static void fatal(const char *reason, unsigned pc, unsigned op) {
    fprintf(stderr, "fatal: %s pc=%x op=%x\n", reason, pc, op);
    exit(3);
}
static uint32_t entropy(void) { return seed; }
static const ZPlatform platform = {text, fatal, entropy, NULL, NULL};

static bool command(const char *line) {
    last_size = 0;
    last_text[0] = 0;
    if (!z_submit(line)) return false;
    return z_run(2000000) == Z_PROMPT;
}

static bool lit(void) { return z_global(lit_globals[chapter]) != 0; }
static void mark(unsigned room) { if (room < ROOMS) visited[room >> 3] |= 1u << (room & 7); }

// Breadth-first search over size-1 (static) exit properties of room objects.
static unsigned find_path(unsigned from, unsigned to, uint8_t dirs[MAX_PATH]) {
    static int previous[ROOMS], via[ROOMS];
    unsigned queue[ROOMS], head = 0, tail = 0;
    for (unsigned i = 0; i < ROOMS; i++) previous[i] = -1;
    previous[from] = (int)from;
    queue[tail++] = from;
    while (head < tail) {
        unsigned room = queue[head++];
        if (room == to) break;
        for (unsigned d = 0; d < 12; d++) {
            const uint8_t *exit;
            if (z_property(room, direction_props[d], &exit) != 1) continue;
            unsigned next = exit[0];
            if (!next || next >= ROOMS || previous[next] >= 0) continue;
            previous[next] = (int)room;
            via[next] = (int)d;
            queue[tail++] = next;
        }
    }
    if (previous[to] < 0) return 0;
    unsigned count = 0;
    uint8_t reversed[MAX_PATH];
    for (unsigned room = to; room != from && count < MAX_PATH; room = (unsigned)previous[room])
        reversed[count++] = (uint8_t)via[room];
    for (unsigned i = 0; i < count; i++) dirs[i] = reversed[count - 1 - i];
    return count;
}

int main(int argc, char **argv) {
    if (argc < 3) { fprintf(stderr, "usage: route-saves CHAPTER ROOM [ROOM...]\n"); return 2; }
    chapter = (unsigned)atoi(argv[1]) - 1;
    assert(chapter < 3);
    seed = seeds[chapter];
    char path[256];
    snprintf(path, sizeof(path), "stories/zork%u.z3", chapter + 1);
    FILE *f = fopen(path, "rb");
    assert(f);
    size_t size = fread(story, 1, sizeof(story), f);
    fclose(f);
    snprintf(path, sizeof(path), "tests/fixtures/zork%u-winning-route.tsv", chapter + 1);
    f = fopen(path, "r");
    assert(f);
    char line[1024];
    while (fgets(line, sizeof(line), f) && route_count < MAX_ROUTE) {
        if (line[0] == '#' || line[0] == '\n') continue;
        char *cmd = strchr(line, '\t');
        cmd = cmd ? strchr(cmd + 1, '\t') : NULL;
        if (!cmd) continue;
        cmd++;
        cmd[strcspn(cmd, "\r\n")] = 0;
        snprintf(route[route_count++], sizeof(route[0]), "%s", cmd);
    }
    fclose(f);

    unsigned targets = (unsigned)argc - 2, done = 0;
    bool found[64] = {false};
    z_start(story, size, &platform);
    assert(z_run(2000000) == Z_PROMPT);
    for (unsigned step = 0; step <= route_count && done < targets; step++) {
        if (step) {
            if (!command(route[step - 1])) break;   // story ended
        }
        mark(z_global(0));
        memset(snapshot, 0, sizeof(snapshot));
        if (!save_write(snapshot, chapter, visited, &trail, NULL)) continue;
        for (unsigned t = 0; t < targets; t++) {
            if (found[t]) continue;
            // A spec is a room ID, or ROOM=step;step;... where ">N" walks to room N
            // over static exits and any other step is an ordinary story command.
            const char *spec = argv[t + 2];
            unsigned target = (unsigned)atoi(spec);
            const char *recipe = strchr(spec, '=');
            char plan[512];
            if (recipe) snprintf(plan, sizeof(plan), "%s", recipe + 1);
            else snprintf(plan, sizeof(plan), ">%u", target);
            static char log[64][64];
            unsigned logged = 0;
            bool ok = true;
            for (char *item = strtok(plan, ";"); item && ok; item = strtok(NULL, ";")) {
                if (*item == '>') {
                    unsigned to = (unsigned)atoi(item + 1), here = z_global(0);
                    uint8_t dirs[MAX_PATH];
                    unsigned steps = here == to ? 0 : find_path(here, to, dirs);
                    if (here != to && !steps) { ok = false; break; }
                    for (unsigned i = 0; i < steps && ok; i++) {
                        ok = command(direction_words[dirs[i]]);
                        if (logged < 64) snprintf(log[logged++], sizeof(log[0]), "%s", direction_words[dirs[i]]);
                        if (getenv("ROUTE_DEBUG") && (unsigned)atoi(getenv("ROUTE_DEBUG")) == step)
                            fprintf(stderr, "[step %u] %s -> room %u: %.160s\n", step, direction_words[dirs[i]], z_global(0), last_text);
                    }
                } else {
                    ok = command(item);
                    if (logged < 64) snprintf(log[logged++], sizeof(log[0]), "%s", item);
                    if (getenv("ROUTE_DEBUG") && (unsigned)atoi(getenv("ROUTE_DEBUG")) == step)
                        fprintf(stderr, "[step %u] %s -> room %u: %.300s\n", step, item, z_global(0), last_text);
                }
            }
            if (ok && z_global(0) == target && lit()) {
                memset(output, 0, sizeof(output));
                uint8_t seen[VISITED_BYTES];
                memcpy(seen, visited, sizeof(seen));
                seen[target >> 3] |= 1u << (target & 7);
                assert(save_write(output, chapter, seen, &trail, NULL));
                char name[160];
                snprintf(name, sizeof(name), "qa/mgba-room-gallery/checkpoints/zork-%u-room-%u", chapter + 1, target);
                snprintf(path, sizeof(path), "%s.sav", name);
                FILE *out = fopen(path, "wb");
                assert(out && fwrite(output, 1, sizeof(output), out) == sizeof(output));
                fclose(out);
                snprintf(path, sizeof(path), "%s.txt", name);
                out = fopen(path, "w");
                assert(out);
                fprintf(out, "# Zork %u room %u: winning-route fixture steps 1-%u (seed %u), then:\n",
                        chapter + 1, target, step, seed);
                for (unsigned i = 0; i < logged; i++) fprintf(out, "%s\n", log[i]);
                fprintf(out, "# Arrived lit at room %u. Story text on arrival:\n", target);
                for (char *p = last_text; *p; ) {
                    char *e = strchr(p, '\n');
                    fprintf(out, "#   %.*s\n", e ? (int)(e - p) : (int)strlen(p), p);
                    if (!e) break;
                    p = e + 1;
                }
                fclose(out);
                found[t] = true;
                done++;
                printf("room %u: saved after route step %u + %u commands\n", target, step, logged);
            }
            // Return to the route snapshot before trying the next room or step.
            assert(save_read(snapshot, chapter, visited, &trail, NULL));
        }
    }
    for (unsigned t = 0; t < targets; t++)
        if (!found[t]) printf("room %s: NOT reached from any route step\n", argv[t + 2]);
    return done == targets ? 0 : 1;
}
