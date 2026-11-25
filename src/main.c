#include "adventure.h"
#include "video.h"

enum { ART_TOP = 15, TEXT_TOP = 117, FOOT_TOP = 145 };
#define INK RGB(17, 23, 21)
#define PANEL RGB(32, 43, 37)
#define LINE RGB(102, 102, 70)
#define PAPER RGB(247, 231, 194)
#define GOLD RGB(248, 193, 103)
#define DIM RGB(173, 182, 153)
#define SHADOW RGB(8, 11, 10)

extern const Color art_west[], art_north[], art_south[], art_rear[], art_kitchen[];
extern const Color art_living[], art_living_moved[], art_cellar[];
static const Color *const art[ROOM_COUNT] = {
    art_west, art_north, art_south, art_rear, art_kitchen, art_living, art_cellar
};

typedef enum { TITLE, EXPLORE, ACTIONS, INVENTORY, CONTAINERS, PAUSE, MAP, RESTART } Screen;
typedef struct { int x, y; } Point;

static Adventure game;
static Screen screen;
static int focus, selection, bag_selection;
static bool from_pack;
static Item active_item;

static void badge(int x, int y, const char *key, const char *action) {
    int w = label_width(key) + 6;
    box(x, y, w, 11, PANEL, LINE);
    label(x + 3, y + 2, key, GOLD);
    label(x + w + 4, y + 2, action, PAPER);
}

static void footer(void) {
    fill(0, FOOT_TOP, SCREEN_W, SCREEN_H - FOOT_TOP, INK);
    fill(0, FOOT_TOP, SCREEN_W, 1, LINE);
}

static void header(const char *name, const char *status) {
    fill(0, 0, SCREEN_W, ART_TOP, INK);
    fill(0, ART_TOP - 1, SCREEN_W, 1, LINE);
    fill(6, 4, 2, 7, GOLD);
    label(12, 4, name, PAPER);
    if (status) label(232 - label_width(status), 4, status, GOLD);
}

static void room_header(const char *name, const char *status) {
    static const char *const directions[] = {"N", "E", "S", "W"};
    fill(0, 0, SCREEN_W, ART_TOP, INK);
    fill(0, ART_TOP - 1, SCREEN_W, 1, LINE);
    fill(6, 4, 2, 7, GOLD);

    int status_width = status ? label_width(status) : 0;
    int right_edge = status ? 232 - status_width - 6 : 232;
    int path_width = 0, count = 0;
    for (Direction dir = NORTH; dir < DIRECTION_COUNT; dir++) {
        if (rooms[game.room].exits[dir] == NO_ROOM) continue;
        path_width += label_width(directions[dir]) + (count ? 4 : 0);
        count++;
    }
    int x = right_edge - path_width;
    for (Direction dir = NORTH; dir < DIRECTION_COUNT; dir++) {
        if (rooms[game.room].exits[dir] == NO_ROOM) continue;
        bool blocked = window_blocks(&game, dir);
        label(x, 4, directions[dir], blocked ? DIM : GOLD);
        if (blocked) fill(x, 12, label_width(directions[dir]), 1, DIM);
        x += label_width(directions[dir]) + 4;
    }
    if (status) label(232 - status_width, 4, status, GOLD);

    int name_width = (count ? right_edge - path_width - 6 : right_edge - 6) - 12;
    if (name_width < 0) name_width = 0;
    paragraph(12, 4, name_width, 1, name, PAPER);
}

static void heading_screen(void) {
    header("THE GREAT UNDERGROUND EMPIRE", 0);
    scene(ART_TOP, art_west);
    box(8, 24, 113, 42, INK, LINE);
    title(15, 31, "ZORK I", GOLD, 3);
    label(16, 55, "THE WHITE HOUSE", PAPER);
    fill(0, TEXT_TOP, SCREEN_W, FOOT_TOP - TEXT_TOP, PANEL);
    fill(0, TEXT_TOP, SCREEN_W, 1, LINE);
    label(8, 125, "A handheld adventure based on Infocom's Zork.", PAPER);
    footer();
    badge(8, 148, "A / START", "Begin");
}

static Point item_position(Item item) {
    if (item >= LEAFLET) {
        unsigned place = game.place[item];
        if (place == IN_MAILBOX) return (Point){55, 90};
        if (place == IN_CASE) return (Point){193, 56};
        if (place == IN_SACK) return item_position(SACK);
        if (game.room == KITCHEN && item == SACK) return (Point){89, 68};
        if (game.room == KITCHEN && item == BOTTLE) return (Point){126, 68};
        if (game.room == LIVING && item == LANTERN) return (Point){161, 66};
        if (game.room == LIVING && item == SWORD) return (Point){156, 42};
        return (Point){36 + (item - LEAFLET) * 24, 89};
    }
    switch (item) {
        case MAILBOX: return (Point){60, 90};
        case BOARDED_DOOR: return (Point){139, 68};
        case BOARDED_WINDOWS: return (Point){game.room == HOUSE_NORTH ? 205 : 48, 62};
        case WINDOW: return game.room == HOUSE_REAR ? (Point){107, 63} : (Point){202, 57};
        case CASE: return (Point){192, 58};
        case RUG: return (Point){game.rug_moved ? 57 : 133, 94};
        case HATCH: return (Point){127, 96};
        case STAIRS: return (Point){203, 81};
        default: return (Point){36, 88};
    }
}

static void mailbox_sprite(Point p) {
    int x = p.x - 8, y = p.y - 8;
    fill(x + 6, y + 10, 4, 15, SHADOW);
    fill(x + 7, y + 10, 2, 14, RGB(117, 81, 50));
    box(x, y + 2, 17, 9, RGB(102, 119, 113), SHADOW);
    fill(x + 3, y + 2, 11, 1, DIM);
    if (game.mailbox_open) {
        fill(x + 1, y + 5, 4, 5, SHADOW);
        fill(x - 2, y + 10, 6, 2, DIM);
        if (game.place[LEAFLET] == IN_MAILBOX) fill(x + 1, y + 8, 3, 1, PAPER);
    } else {
        fill(x + 1, y + 4, 3, 5, LINE);
        fill(x + 9, y + 4, 6, 2, RGB(177, 68, 43));
    }
}

static void item_sprite(Item item, Point p) {
    int x = p.x, y = p.y;
    switch (item) {
        case LANTERN:
            box(x - 4, y - 6, 9, 12, game.lantern_on ? GOLD : RGB(142, 100, 55), SHADOW);
            fill(x - 2, y - 8, 5, 2, LINE);
            fill(x - 1, y - 4, 3, 7, game.lantern_on ? PAPER : RGB(208, 157, 85));
            fill(x - 5, y + 5, 11, 2, SHADOW);
            break;
        case LEAFLET:
            box(x - 4, y - 3, 9, 7, PAPER, LINE);
            fill(x - 2, y - 1, 5, 1, DIM);
            break;
        case SACK:
            box(x - 7, y - 7, 14, 15, RGB(173, 132, 83), SHADOW);
            fill(x - 5, y - 6, 10, 2, game.sack_open ? SHADOW : GOLD);
            break;
        case BOTTLE:
            box(x - 3, y - 5, 7, 12, RGB(93, 140, 130), SHADOW);
            fill(x - 1, y - 8, 3, 4, DIM);
            fill(x - 1, y - 3, 1, 6, PAPER);
            break;
        case SWORD:
            for (int i = 0; i < 18; i++) pixel(x - 12 + i, y + 5 - i / 2, PAPER);
            fill(x + 1, y - 4, 2, 7, GOLD);
            fill(x + 3, y - 3, 7, 2, RGB(141, 102, 55));
            break;
        default: break;
    }
}

static void mark(Point p) {
    int x = p.x - 5, y = p.y - 5;
    fill(x - 1, y, 13, 1, SHADOW);
    fill(x - 1, y + 10, 13, 1, SHADOW);
    fill(x, y, 11, 1, GOLD);
    fill(x, y + 10, 11, 1, GOLD);
    fill(x, y + 1, 1, 3, GOLD);
    fill(x + 10, y + 1, 1, 3, GOLD);
    fill(x, y + 7, 1, 3, GOLD);
    fill(x + 10, y + 7, 1, 3, GOLD);
}

static void draw_room(void) {
    bool dark = cellar_dark(&game);
    room_header(rooms[game.room].name, game.lantern_on && held(&game, LANTERN) ? "LANTERN ON" : 0);
    if (dark) {
        fill(0, ART_TOP, SCREEN_W, SCENE_H, SHADOW);
    } else {
        const Color *background = game.room == LIVING && game.rug_moved ? art_living_moved : art[game.room];
        scene(ART_TOP, background);
        if (game.room == HOUSE_WEST) mailbox_sprite(item_position(MAILBOX));
        if (game.room == LIVING && game.hatch_open) {
            for (int row = 0; row < 28; row++) {
                int w = 78 + row * 3 / 4;
                fill(126 - w / 2, 87 + row, w, 1, RGB(126, 83, 47));
                fill(128 - w / 2, 87 + row, w - 4, 1, SHADOW);
            }
            for (int i = 0; i < 4; i++) fill(104 - i, 93 + 5 * i, 45 + 2 * i, 2, RGB(102, 71, 46));
        }
    }
    Item here[ITEM_COUNT];
    int count = objects_here(&game, here);
    for (int i = 0; !dark && i < count; i++) {
        Item item = here[i];
        if (item >= LEAFLET && game.place[item] == game.room) item_sprite(item, item_position(item));
    }
    if (count) {
        int current = focus < count ? focus : 0;
        Item item = here[current];
        if (!dark) mark(item_position(item));
        const char *name = item_names[item];
        int w = label_width(name) + 18 + count * 5;
        box(6, 101, w, 14, INK, LINE);
        label(12, 105, name, GOLD);
        for (int i = 0; i < count; i++) fill(w - count * 5 + 4 + 5 * i, 107, 3, 3, i == current ? GOLD : LINE);
    }
    fill(0, TEXT_TOP, SCREEN_W, FOOT_TOP - TEXT_TOP, PANEL);
    fill(0, TEXT_TOP, SCREEN_W, 1, LINE);
    paragraph(8, 122, 224, 2, game.message, PAPER);
    footer();
    badge(6, 148, "PAD", "Walk");
    badge(57, 148, "A", "Act");
    badge(92, 148, "B", "Look");
    badge(132, 148, "L/R", "Focus");
    badge(187, 148, "SEL", "Bag");
}

static void menu(const char *name, const char *const *options, int count, int current) {
    int w = 117, x = 117, h = 22 + (count ? count : 1) * 13;
    box(x + 2, 26, w, h, SHADOW, SHADOW);
    box(x, 24, w, h, INK, GOLD);
    label(x + 8, 30, name, GOLD);
    fill(x + 7, 42, w - 14, 1, LINE);
    if (!count) label(x + 9, 48, "Nothing carried.", DIM);
    for (int i = 0; i < count; i++) {
        int y = 46 + i * 13;
        if (i == current) {
            fill(x + 4, y - 1, w - 8, 12, PANEL);
            fill(x + 5, y + 1, 2, 7, GOLD);
        }
        label(x + 11, y + 1, options[i], i == current ? PAPER : DIM);
    }
    footer();
    dpad_badge(6, 148, 11, KEY_UP, PANEL, LINE, GOLD);
    dpad_badge(19, 148, 11, KEY_DOWN, PANEL, LINE, GOLD);
    label(34, 150, "Choose", PAPER);
    badge(69, 148, "A", "Confirm");
    badge(132, 148, "B", "Back");
}

static const Point map_points[ROOM_COUNT] = {
    {35, 62}, {119, 32}, {119, 85}, {202, 62}, {202, 107}, {99, 107}, {99, 130}
};
static const char *const map_names[ROOM_COUNT] = {"West", "North", "South", "Behind", "Kitchen", "Living room", "Cellar"};

static void map_edge(Point a, Point b, Color color) {
    int step_x = a.x < b.x ? 1 : -1;
    int step_y = a.y < b.y ? 1 : -1;
    int dx = (b.x - a.x) * step_x;
    int dy = (a.y - b.y) * step_y;
    int error = dx + dy;
    for (;;) {
        pixel(a.x, a.y, color);
        if (a.x == b.x && a.y == b.y) break;
        int twice = 2 * error;
        if (twice >= dy) {
            error += dy;
            a.x += step_x;
        }
        if (twice <= dx) {
            error += dx;
            a.y += step_y;
        }
    }
}

static bool visited(Room room) {
    return room != NO_ROOM && (game.visited & (1u << room));
}

static void draw_map(void) {
    header("Places visited", 0);
    fill(0, ART_TOP, SCREEN_W, FOOT_TOP - ART_TOP, PANEL);
    for (Room room = HOUSE_WEST; room < ROOM_COUNT; room++) {
        if (!visited(room)) continue;
        for (Direction dir = NORTH; dir < DIRECTION_COUNT; dir++) {
            Room next = rooms[room].exits[dir];
            if (next < room && visited(next)) map_edge(map_points[room], map_points[next], LINE);
        }
    }
    if (visited(CELLAR)) map_edge(map_points[LIVING], map_points[CELLAR], LINE);
    for (Room room = HOUSE_WEST; room < ROOM_COUNT; room++) {
        if (!visited(room)) continue;
        const Point p = map_points[room];
        const char *name = map_names[room];
        Color color = game.room == room ? GOLD : DIM;
        int w = label_width(name) + 12;
        box(p.x - w / 2, p.y - 7, w, 15, INK, color);
        label(p.x - w / 2 + 6, p.y - 3, name, color);
    }
    footer();
    badge(6, 148, "B", "Back");
    badge(45, 148, "START", "Resume");
}

static const char *const pause_options[] = {"Resume", "Places visited", "Restart demo"};
static const char *const restart_options[] = {"Keep exploring", "Start over"};

static void render(void) {
    if (screen == TITLE) {
        heading_screen();
        return;
    }
    if (screen == MAP) {
        draw_map();
        return;
    }
    draw_room();
    const char *options[ITEM_COUNT];
    if (screen == ACTIONS) {
        Verb verbs[5];
        int count = verbs_for(&game, active_item, verbs);
        for (int i = 0; i < count; i++) options[i] = verb_names[verbs[i]];
        menu(item_names[active_item], options, count, selection);
    } else if (screen == INVENTORY || screen == CONTAINERS) {
        Item items[ITEM_COUNT];
        int count = screen == INVENTORY ? inventory(&game, items) : containers_for(&game, active_item, items);
        for (int i = 0; i < count; i++) options[i] = item_names[items[i]];
        menu(screen == INVENTORY ? "Carried items" : "Put it where?", options, count, selection);
    } else if (screen == PAUSE) {
        menu("Paused", pause_options, 3, selection);
        fill(0, TEXT_TOP + 1, SCREEN_W, FOOT_TOP - TEXT_TOP - 1, PANEL);
        paragraph(8, 122, 224, 2, "D-pad moves. L/R focus. A opens verbs.\nB looks. Select opens your bag.", PAPER);
    } else if (screen == RESTART) {
        menu("Restart demo?", restart_options, 2, selection);
    }
}

static void rotate(int *index, int count, int step) {
    if (!count) return;
    *index += step;
    if (*index >= count) *index = 0;
    if (*index < 0) *index = count - 1;
}

static void choose_item(Item item, bool pack) {
    active_item = item;
    from_pack = pack;
    bag_selection = selection;
    screen = ACTIONS;
    selection = 0;
}

static void apply_action(Verb verb, Item target) {
    Room old_room = game.room;
    act(&game, active_item, verb, target);
    Item here[ITEM_COUNT];
    int count = objects_here(&game, here);
    if (old_room != game.room || focus >= count) focus = 0;
    if (verb == OPEN && active_item == MAILBOX && game.place[LEAFLET] == IN_MAILBOX) {
        for (int i = 0; i < count; i++) if (here[i] == LEAFLET) focus = i;
    }
    screen = EXPLORE;
}

static void explore(uint16_t pressed) {
    if (pressed & KEY_START) {
        screen = PAUSE;
        selection = 0;
    } else if (pressed & KEY_SELECT) {
        screen = INVENTORY;
        selection = 0;
    } else if (pressed & KEY_B) {
        look(&game);
    } else if (pressed & (KEY_A | KEY_L | KEY_R)) {
        Item items[ITEM_COUNT];
        int count = objects_here(&game, items);
        if (!count) return;
        if (pressed & KEY_A) choose_item(items[focus], false);
        else rotate(&focus, count, (pressed & KEY_R) ? 1 : -1);
    } else {
        Room old_room = game.room;
        if (pressed & KEY_UP) walk(&game, NORTH);
        else if (pressed & KEY_RIGHT) walk(&game, EAST);
        else if (pressed & KEY_DOWN) walk(&game, SOUTH);
        else if (pressed & KEY_LEFT) walk(&game, WEST);
        if (old_room != game.room) focus = 0;
    }
}

static void update(uint16_t pressed) {
    if (screen == TITLE) {
        if (pressed & (KEY_A | KEY_START)) {
            adventure_start(&game);
            screen = EXPLORE;
        }
        return;
    }
    if (screen == EXPLORE) {
        explore(pressed);
        return;
    }
    if (screen == MAP) {
        if (pressed & KEY_START) screen = EXPLORE;
        else if (pressed & KEY_B) screen = PAUSE;
        return;
    }
    if (pressed & KEY_B) {
        if (screen == CONTAINERS) {
            Verb verbs[5];
            selection = verbs_for(&game, active_item, verbs) - 1;
            screen = ACTIONS;
        } else if (screen == ACTIONS && from_pack) {
            selection = bag_selection;
            screen = INVENTORY;
        } else if (screen == RESTART) {
            selection = 2;
            screen = PAUSE;
        } else {
            screen = EXPLORE;
        }
        return;
    }
    if (screen == PAUSE && (pressed & KEY_START)) {
        screen = EXPLORE;
        return;
    }

    Item items[ITEM_COUNT];
    Verb verbs[5];
    int count = 0;
    switch (screen) {
        case ACTIONS: count = verbs_for(&game, active_item, verbs); break;
        case INVENTORY: count = inventory(&game, items); break;
        case CONTAINERS: count = containers_for(&game, active_item, items); break;
        case PAUSE: count = 3; break;
        case RESTART: count = 2; break;
        default: break;
    }
    if (pressed & KEY_UP) rotate(&selection, count, -1);
    else if (pressed & KEY_DOWN) rotate(&selection, count, 1);
    else if ((pressed & KEY_A) && count) {
        switch (screen) {
            case ACTIONS:
                if (verbs[selection] == PUT_IN) {
                    selection = 0;
                    screen = CONTAINERS;
                } else {
                    apply_action(verbs[selection], NO_ITEM);
                }
                break;
            case INVENTORY: choose_item(items[selection], true); break;
            case CONTAINERS: apply_action(PUT_IN, items[selection]); break;
            case PAUSE:
                if (selection == 0) screen = EXPLORE;
                else if (selection == 1) screen = MAP;
                else {
                    screen = RESTART;
                    selection = 0;
                }
                break;
            case RESTART:
                if (selection == 1) {
                    adventure_start(&game);
                    focus = 0;
                    screen = EXPLORE;
                } else {
                    selection = 2;
                    screen = PAUSE;
                }
                break;
            default: break;
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
        uint16_t held_keys = buttons();
        uint16_t pressed = held_keys & ~previous;
        previous = held_keys;
        if (pressed) {
            update(pressed);
            render();
            present();
        }
    }
}
