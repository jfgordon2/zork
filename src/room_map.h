#ifndef ZORK_ROOM_MAP_H
#define ZORK_ROOM_MAP_H

#include <stdbool.h>
#include <stdint.h>

enum { MAP_CELLS = 9, MAP_CENTER = 4, MAP_NO_DIRECTION = 12 };

unsigned map_direction(bool vertical, unsigned cell);
void draw_room_map(const uint8_t rooms[MAP_CELLS], bool vertical, unsigned cursor, unsigned current);

#endif
