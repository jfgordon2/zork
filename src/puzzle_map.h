#ifndef ZORK_PUZZLE_MAP_H
#define ZORK_PUZZLE_MAP_H

#include <stdbool.h>
#include <stdint.h>

typedef enum { PUZZLE_FLOOR, PUZZLE_MARBLE, PUZZLE_SANDSTONE, PUZZLE_UNKNOWN, PUZZLE_PLAYER } PuzzleTile;
typedef struct { PuzzleTile tiles[9]; bool valid; } PuzzleMap;

void puzzle_map_update(PuzzleMap *map, char *response, unsigned *size);

#endif
