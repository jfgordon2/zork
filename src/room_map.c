#include "room_map.h"
#include "chapter.h"

#define INK RGB(18, 25, 23)
#define PANEL RGB(31, 43, 38)
#define ROOM RGB(42, 56, 49)
#define GRID RGB(48, 64, 54)
#define LINE RGB(113, 124, 99)
#define GOLD RGB(249, 195, 109)
#define PAPER RGB(250, 233, 201)
#define DIM RGB(172, 184, 164)

enum { CELL_WIDTH = 62, CELL_HEIGHT = 24 };

unsigned map_direction(bool vertical, unsigned cell) {
    static const uint8_t directions[2][MAP_CELLS] = {
        {7, 0, 4, 3, MAP_NO_DIRECTION, 1, 6, 2, 5},
        {MAP_NO_DIRECTION, 8, MAP_NO_DIRECTION, 11, MAP_NO_DIRECTION, 10, MAP_NO_DIRECTION, 9, MAP_NO_DIRECTION}
    };
    return directions[vertical][cell];
}

static int node_x(unsigned cell) { return 8 + (int)(cell % 3) * 81; }
static int node_y(unsigned cell) { return 21 + (int)(cell / 3) * 34; }

static void route_line(int x, int y, int tx, int ty) {
    int dx = tx > x ? tx - x : x - tx, dy = ty > y ? ty - y : y - ty;
    int step_x = x < tx ? 1 : -1, step_y = y < ty ? 1 : -1;
    int error = dx - dy;
    int steps = dx > dy ? dx : dy;
    for (int i = 0; i <= steps; i++) {
        pixel(x, y, LINE);
        if (x == tx && y == ty) return;
        int twice = 2 * error;
        if (twice > -dy) { error -= dy; x += step_x; }
        if (twice < dx) { error += dx; y += step_y; }
    }
}

static void route_badge(bool vertical, unsigned cell, int x, int y) {
    unsigned direction = map_direction(vertical, cell);
    static const char *const names[] = {"N", "E", "S", "W", "NE", "SE", "SW", "NW", "Up", "Down", "In", "Out"};
    if (direction >= MAP_NO_DIRECTION) return;
    const char *name = names[direction];
    int width = label_width(name) + 6;
    fill(x - width / 2, y - 4, width, 9, PANEL);
    label(x - width / 2 + 3, y - 3, name, DIM);
}

void draw_room_map(const uint8_t rooms[MAP_CELLS], bool vertical, unsigned cursor, unsigned current) {
    fill(0, 15, SCREEN_W, 130, PANEL);
    for (int x = 8; x < 236; x += 8) {
        for (int y = 19; y < 116; y += 8) pixel(x, y, GRID);
    }
    if (!rooms[MAP_CENTER]) {
        label(12, 53, "Explore a lit room to start mapping.", PAPER);
        return;
    }

    int cx = node_x(MAP_CENTER) + CELL_WIDTH / 2;
    int cy = node_y(MAP_CENTER) + CELL_HEIGHT / 2;
    for (unsigned i = 0; i < MAP_CELLS; i++) {
        if (!rooms[i] || i == MAP_CENTER) continue;
        int x = node_x(i) + CELL_WIDTH / 2;
        int y = node_y(i) + CELL_HEIGHT / 2;
        route_line(cx, cy, x, y);
        route_badge(vertical, i, (cx + x) / 2, (cy + y) / 2);
    }

    char name[Z_NAME_MAX];
    for (unsigned i = 0; i < MAP_CELLS; i++) {
        int x = node_x(i), y = node_y(i);
        if (!rooms[i]) {
            if (cursor == i) box(x + 23, y + 6, 16, 12, PANEL, LINE);
            continue;
        }
        bool here = rooms[i] == current;
        Color edge = here || cursor == i ? GOLD : LINE;
        box(x, y, CELL_WIDTH, CELL_HEIGHT, i == MAP_CENTER ? INK : ROOM, edge);
        if (here) {
            fill(x + 2, y + 3, 2, CELL_HEIGHT - 6, GOLD);
        }
        z_name(rooms[i], name, sizeof(name));
        paragraph(x + 7, y + 3, CELL_WIDTH - 11, 2, name, here ? GOLD : PAPER);
    }

    unsigned room = rooms[cursor];
    if (room) z_name(room, name, sizeof(name));
    else {
        name[0] = 0;
        unsigned direction = map_direction(vertical, cursor);
        append_text(name, sizeof(name), direction < MAP_NO_DIRECTION ? "No mapped route " : "No room here");
        if (direction < MAP_NO_DIRECTION) append_text(name, sizeof(name), path_names[direction]);
    }
    box(8, 123, 224, 17, INK, LINE);
    paragraph(15, 128, room == current ? 146 : 210, 1, name, PAPER);
    if (room == current) label(164, 128, "You are here", GOLD);
}
