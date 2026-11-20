#include "puzzle_map.h"
#include "chapter.h"

static const char *line_end(const char *line) {
    while (*line && *line != '\n') line++;
    return line;
}

static char *find_text(char *text, const char *part) {
    for (; *text; text++) {
        unsigned i = 0;
        while (part[i] && text[i] == part[i]) i++;
        if (!part[i]) return text;
    }
    return NULL;
}

static void remove_text(char *start, const char *end, unsigned *size) {
    *size -= end - start;
    while ((*start++ = *end++) != 0) {}
}

static bool decode_row(const char *line, const char *end, PuzzleTile tiles[3]) {
    while (line < end && *line != '+') line++;
    if (end - line < 10 || line[9] != '+') return false;
    for (unsigned col = 0; col < 3; col++) {
        const char *pair = line + 1 + col * 3;
        if ((col && pair[-1] != ' ') || pair[0] != pair[1]) return false;
        switch (*pair) {
            case ' ': tiles[col] = PUZZLE_FLOOR; break;
            case 'M': tiles[col] = PUZZLE_MARBLE; break;
            case 'S': tiles[col] = PUZZLE_SANDSTONE; break;
            case '?': tiles[col] = PUZZLE_UNKNOWN; break;
            case '.': tiles[col] = PUZZLE_PLAYER; break;
            default: return false;
        }
    }
    return true;
}

void puzzle_map_update(PuzzleMap *map, char *response, unsigned *size) {
    char *line = response;
    bool decoded = false;
    while (*line) {
        const char *end = line;
        PuzzleMap grid = {0};
        bool valid = true;
        for (unsigned row = 0; row < 3; row++) {
            const char *next = line_end(end);
            if (!decode_row(end, next, grid.tiles + row * 3)) {
                valid = false;
                break;
            }
            end = *next ? next + 1 : next;
        }
        if (valid && grid.tiles[4] == PUZZLE_PLAYER) {
            grid.valid = true;
            *map = grid;
            decoded = true;
            remove_text(line, end, size);
            continue;
        }
        const char *next = line_end(line);
        line += next - line + (*next != 0);
    }

    if (decoded) {
        char *intro = find_text(response, "The architecture of this region is getting complex");
        const char *legend_end = intro ? find_text(intro, "?? = unknown (blocked by walls)") : NULL;
        if (legend_end) {
            legend_end = line_end(legend_end);
            while (*legend_end == '\n') legend_end++;
            remove_text(intro, legend_end, size);
        }
    }

    if (!map->valid && text_has(response, "north and west with marble walls and to the east and south with sandstone")) {
        *map = (PuzzleMap){{PUZZLE_UNKNOWN, PUZZLE_MARBLE, PUZZLE_UNKNOWN,
                            PUZZLE_MARBLE, PUZZLE_PLAYER, PUZZLE_SANDSTONE,
                            PUZZLE_UNKNOWN, PUZZLE_SANDSTONE, PUZZLE_UNKNOWN}, true};
    }
}
