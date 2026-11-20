#ifndef ADVENTURE_H
#define ADVENTURE_H

#include <stdbool.h>
#include <stdint.h>

typedef enum { NORTH, EAST, SOUTH, WEST, DIRECTION_COUNT } Direction;
typedef enum {
    HOUSE_WEST, HOUSE_NORTH, HOUSE_SOUTH, HOUSE_REAR, KITCHEN, LIVING, CELLAR,
    ROOM_COUNT, NO_ROOM = -1
} Room;
typedef enum {
    MAILBOX, BOARDED_DOOR, BOARDED_WINDOWS, WINDOW, CASE, RUG, HATCH, STAIRS,
    LEAFLET, SACK, BOTTLE, LANTERN, SWORD, ITEM_COUNT, NO_ITEM = -1
} Item;
typedef enum {
    EXAMINE, OPEN, CLOSE, READ, TAKE, DROP, TURN_ON, TURN_OFF,
    LOOK_UNDER, MOVE, CLIMB_IN, CLIMB_OUT, DESCEND, ASCEND, PUT_IN
} Verb;
typedef enum { PACK = ROOM_COUNT, IN_MAILBOX, IN_CASE, IN_SACK } Place;

typedef struct {
    const char *name;
    const char *description;
    Room exits[DIRECTION_COUNT];
} RoomInfo;

typedef struct {
    Room room;
    unsigned visited;
    uint8_t place[ITEM_COUNT];
    bool mailbox_open, window_open, case_open, sack_open, rug_moved, hatch_open;
    bool lantern_on, cellar_seen;
    const char *message;
} Adventure;

extern const RoomInfo rooms[ROOM_COUNT];
extern const char *const item_names[ITEM_COUNT];
extern const char *const verb_names[];

void adventure_start(Adventure *game);
void look(Adventure *game);
void walk(Adventure *game, Direction direction);
bool cellar_dark(const Adventure *game);
bool window_blocks(const Adventure *game, Direction direction);
bool held(const Adventure *game, Item item);
int objects_here(const Adventure *game, Item list[ITEM_COUNT]);
int inventory(const Adventure *game, Item list[ITEM_COUNT]);
int verbs_for(const Adventure *game, Item item, Verb list[5]);
int containers_for(const Adventure *game, Item item, Item list[3]);
void act(Adventure *game, Item item, Verb verb, Item target);

#endif
