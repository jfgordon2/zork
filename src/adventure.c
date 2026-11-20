#include "adventure.h"

const RoomInfo rooms[ROOM_COUNT] = {
    [HOUSE_WEST] = {"West of House", "A white house stands in a field. Its front door is boarded shut.",
                    {HOUSE_NORTH, NO_ROOM, HOUSE_SOUTH, NO_ROOM}},
    [HOUSE_NORTH] = {"North of House", "A path skirts the house. Boards cover the side windows.",
                     {NO_ROOM, HOUSE_REAR, NO_ROOM, HOUSE_WEST}},
    [HOUSE_SOUTH] = {"South of House", "Dry grass brushes against the boarded windows.",
                     {NO_ROOM, HOUSE_REAR, NO_ROOM, HOUSE_WEST}},
    [HOUSE_REAR] = {"Behind House", "A small kitchen window sits crooked in its frame.",
                    {HOUSE_NORTH, NO_ROOM, HOUSE_SOUTH, KITCHEN}},
    [KITCHEN] = {"Kitchen", "Wind stirs the dust in the cold kitchen.",
                 {NO_ROOM, HOUSE_REAR, NO_ROOM, LIVING}},
    [LIVING] = {"Living Room", "An old rug lies before an empty trophy case. The west door is nailed shut.",
                {NO_ROOM, KITCHEN, NO_ROOM, NO_ROOM}},
    [CELLAR] = {"Cellar", "The lantern lights damp stone. A passage leads deeper underground.",
                {NO_ROOM, NO_ROOM, NO_ROOM, NO_ROOM}},
};

const char *const item_names[ITEM_COUNT] = {
    [MAILBOX] = "Small mailbox", [BOARDED_DOOR] = "Boarded door",
    [BOARDED_WINDOWS] = "Boarded windows", [WINDOW] = "Kitchen window",
    [CASE] = "Trophy case", [RUG] = "Woven rug", [HATCH] = "Trap door",
    [STAIRS] = "Rickety stairs", [LEAFLET] = "Leaflet", [SACK] = "Brown sack",
    [BOTTLE] = "Glass bottle", [LANTERN] = "Brass lantern", [SWORD] = "Elvish sword",
};

const char *const verb_names[] = {
    [EXAMINE] = "Examine", [OPEN] = "Open", [CLOSE] = "Close", [READ] = "Read",
    [TAKE] = "Take", [DROP] = "Set down", [TURN_ON] = "Switch on", [TURN_OFF] = "Switch off",
    [LOOK_UNDER] = "Look under", [MOVE] = "Move", [CLIMB_IN] = "Climb in", [CLIMB_OUT] = "Climb out",
    [DESCEND] = "Descend", [ASCEND] = "Climb up", [PUT_IN] = "Put in...",
};

void adventure_start(Adventure *game) {
    game->room = HOUSE_WEST;
    game->visited = 1u << HOUSE_WEST;
    game->place[LEAFLET] = IN_MAILBOX;
    game->place[SACK] = game->place[BOTTLE] = KITCHEN;
    game->place[LANTERN] = game->place[SWORD] = LIVING;
    game->mailbox_open = game->window_open = game->case_open = game->sack_open = false;
    game->rug_moved = game->hatch_open = game->lantern_on = game->cellar_seen = false;
    look(game);
}

bool held(const Adventure *game, Item item) {
    return item >= LEAFLET && game->place[item] == PACK;
}

bool cellar_dark(const Adventure *game) {
    return game->room == CELLAR &&
        (!game->lantern_on || (!held(game, LANTERN) && game->place[LANTERN] != CELLAR));
}

static bool at_hand(const Adventure *game, Item item) {
    unsigned int place = game->place[item];
    if (place == (unsigned int)game->room) return true;
    if (place == IN_MAILBOX) return game->room == HOUSE_WEST && game->mailbox_open;
    if (place == IN_CASE) return game->room == LIVING && game->case_open;
    return place == IN_SACK && game->sack_open &&
        (held(game, SACK) || at_hand(game, SACK));
}

void look(Adventure *game) {
    game->message = rooms[game->room].description;
    switch (game->room) {
        case HOUSE_REAR:
            game->message = game->window_open ? "The kitchen window is open wide enough to climb through."
                                              : "A small kitchen window is slightly ajar.";
            break;
        case LIVING:
            if (game->rug_moved) game->message = game->hatch_open ?
                "A rickety staircase disappears through the open trap door." :
                "The rug is bunched aside. A trap door is set into the floor.";
            break;
        case CELLAR:
            if (cellar_dark(game)) {
                game->message = game->place[LANTERN] == CELLAR ?
                    "You can feel the stairs and the lantern on the ground." :
                    "It is pitch dark. You can still feel the stairs behind you.";
            }
            break;
        default:
            break;
    }
}

static void arrive(Adventure *game, Room room) {
    game->room = room;
    game->visited |= 1u << room;
    look(game);
}

bool window_blocks(const Adventure *game, Direction direction) {
    return !game->window_open && ((game->room == HOUSE_REAR && direction == WEST) ||
                                  (game->room == KITCHEN && direction == EAST));
}

void walk(Adventure *game, Direction direction) {
    Room next = rooms[game->room].exits[direction];
    if (window_blocks(game, direction)) {
        game->message = "The window needs to open wider.";
    } else if (next != NO_ROOM) {
        arrive(game, next);
    } else if ((game->room == HOUSE_WEST && direction == EAST) ||
               (game->room == LIVING && direction == WEST)) {
        game->message = "The door is nailed shut.";
    } else {
        game->message = "You can't go that way here.";
    }
}

int objects_here(const Adventure *game, Item list[ITEM_COUNT]) {
    int count = 0;
    switch (game->room) {
        case HOUSE_WEST:
            list[count++] = MAILBOX;
            list[count++] = BOARDED_DOOR;
            break;
        case HOUSE_NORTH:
        case HOUSE_SOUTH:
            list[count++] = BOARDED_WINDOWS;
            break;
        case HOUSE_REAR:
        case KITCHEN:
            list[count++] = WINDOW;
            break;
        case LIVING:
            list[count++] = RUG;
            list[count++] = CASE;
            if (game->rug_moved) list[count++] = HATCH;
            break;
        case CELLAR:
            list[count++] = STAIRS;
            if (cellar_dark(game)) {
                if (game->place[LANTERN] == CELLAR) list[count++] = LANTERN;
                return count;
            }
            break;
        default:
            break;
    }
    for (Item item = LEAFLET; item < ITEM_COUNT; item++) {
        if (game->place[item] == IN_SACK && held(game, SACK)) continue;
        if (at_hand(game, item)) list[count++] = item;
    }
    return count;
}

int inventory(const Adventure *game, Item list[ITEM_COUNT]) {
    int count = 0;
    for (Item item = LEAFLET; item < ITEM_COUNT; item++) {
        if (held(game, item) || (game->place[item] == IN_SACK && game->sack_open && held(game, SACK))) {
            list[count++] = item;
        }
    }
    return count;
}

int containers_for(const Adventure *game, Item item, Item list[3]) {
    int count = 0;
    if (game->room == HOUSE_WEST && game->mailbox_open && item == LEAFLET) list[count++] = MAILBOX;
    if (game->room == LIVING && game->case_open) list[count++] = CASE;
    if (game->sack_open && (item == LEAFLET || item == BOTTLE) &&
        (held(game, SACK) || (!cellar_dark(game) && at_hand(game, SACK)))) list[count++] = SACK;
    return count;
}

int verbs_for(const Adventure *game, Item item, Verb list[5]) {
    int count = 0;
    list[count++] = EXAMINE;
    switch (item) {
        case MAILBOX: list[count++] = game->mailbox_open ? CLOSE : OPEN; break;
        case WINDOW:
            list[count++] = game->window_open ? CLOSE : OPEN;
            if (game->window_open) list[count++] = game->room == KITCHEN ? CLIMB_OUT : CLIMB_IN;
            break;
        case CASE: list[count++] = game->case_open ? CLOSE : OPEN; break;
        case RUG:
            if (!game->rug_moved) {
                list[count++] = LOOK_UNDER;
                list[count++] = MOVE;
            }
            break;
        case HATCH:
            list[count++] = game->hatch_open ? CLOSE : OPEN;
            if (game->hatch_open) list[count++] = DESCEND;
            break;
        case STAIRS: list[count++] = ASCEND; break;
        case LEAFLET: list[count++] = READ; break;
        case LANTERN: list[count++] = game->lantern_on ? TURN_OFF : TURN_ON; break;
        case SACK: list[count++] = game->sack_open ? CLOSE : OPEN; break;
        default: break;
    }
    if (item >= LEAFLET) {
        bool carrying = held(game, item);
        list[count++] = carrying ? DROP : TAKE;
        Item targets[3];
        if (carrying && containers_for(game, item, targets)) list[count++] = PUT_IN;
    }
    return count;
}

static const char *examine(const Adventure *game, Item item) {
    switch (item) {
        case MAILBOX:
            if (!game->mailbox_open) return "A small metal mailbox on a post. Its lid is shut.";
            return game->place[LEAFLET] == IN_MAILBOX ? "A folded leaflet lies inside." : "The mailbox is empty.";
        case BOARDED_DOOR: return "The timber over the front door is firmly nailed down.";
        case BOARDED_WINDOWS: return "You can't see through the boards.";
        case WINDOW: return game->window_open ? "The sash is propped open." : "The sash is ajar, but the gap is too narrow.";
        case CASE: return "A glass cabinet for the trophies you meant to bring back.";
        case RUG: return game->rug_moved ? "The rug is bunched against the wall." : "The rug is heavy. One corner sits oddly high.";
        case HATCH: return game->hatch_open ? "Wooden steps lead down into darkness." : "An iron ring is set into the dusty wood.";
        case STAIRS: return "The stairs lead back up to the living room.";
        case LEAFLET: return "A folded leaflet, printed in small, proud lettering.";
        case SACK:
            if (!game->sack_open) return "A crumpled paper sack, folded at the top.";
            return game->place[BOTTLE] == IN_SACK || game->place[LEAFLET] == IN_SACK ?
                "The sack is open. Something is inside." : "The open sack is empty.";
        case BOTTLE: return "A clear glass bottle with water inside.";
        case LANTERN: return game->lantern_on ? "The brass lantern casts a warm light." : "A brass lantern with a small switch.";
        case SWORD: return "A keen old blade with an elvish inscription.";
        default: return "You see nothing unusual.";
    }
}

static const char *set_open(Adventure *game, Item item, bool open) {
    switch (item) {
        case MAILBOX:
            game->mailbox_open = open;
            return !open ? "You close the mailbox." : game->place[LEAFLET] == IN_MAILBOX ?
                "You lift the lid. A folded leaflet is inside." : "You lift the lid. The mailbox is empty.";
        case WINDOW:
            game->window_open = open;
            return open ? "You push the sash open. There's room to climb through." : "The window closes with a scrape.";
        case CASE:
            game->case_open = open;
            return open ? "The glass cabinet creaks open." : "You close the glass cabinet.";
        case SACK:
            game->sack_open = open;
            return open ? "You unfold the top of the sack." : "You fold the sack shut.";
        case HATCH:
            game->hatch_open = open;
            return open ? "You pull up the trap door. Wooden stairs disappear below." : "You lower the trap door.";
        default: return "It won't budge.";
    }
}

static void reveal_cellar(Adventure *game) {
    if (game->room == CELLAR && !cellar_dark(game) && !game->cellar_seen) {
        game->cellar_seen = true;
        game->message = "Your light finds a passage below.\nEnd of the White House demo.";
    }
}

void act(Adventure *game, Item item, Verb verb, Item target) {
    switch (verb) {
        case EXAMINE: game->message = examine(game, item); break;
        case OPEN:
        case CLOSE: game->message = set_open(game, item, verb == OPEN); break;
        case READ: game->message = "WELCOME TO ZORK!\nGreat treasures lie underground."; break;
        case TAKE:
            game->place[item] = PACK;
            game->message = "You take it.";
            break;
        case DROP:
            game->place[item] = game->room;
            game->message = "You set it down.";
            break;
        case TURN_ON:
        case TURN_OFF:
            game->lantern_on = verb == TURN_ON;
            game->message = game->lantern_on ? "The lantern glows amber." : "You switch the lantern off.";
            if (cellar_dark(game)) look(game);
            reveal_cellar(game);
            break;
        case LOOK_UNDER:
            game->message = "You lift a corner. There's a trap door beneath the rug.";
            break;
        case MOVE:
            game->rug_moved = true;
            game->message = "You drag the rug aside, exposing a dusty trap door.";
            break;
        case CLIMB_IN: arrive(game, KITCHEN); break;
        case CLIMB_OUT: arrive(game, HOUSE_REAR); break;
        case DESCEND:
            arrive(game, CELLAR);
            reveal_cellar(game);
            break;
        case ASCEND: arrive(game, LIVING); break;
        case PUT_IN:
            if (target == MAILBOX) game->place[item] = IN_MAILBOX;
            if (target == CASE) game->place[item] = IN_CASE;
            if (target == SACK) game->place[item] = IN_SACK;
            game->message = "You put it inside.";
            break;
    }
}
