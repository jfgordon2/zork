#ifndef ZORK_TRAIL_H
#define ZORK_TRAIL_H

#include <stdint.h>

enum { TRAIL_CAPACITY = 320, TRAIL_DIRECTIONS = 12 };

typedef struct { uint8_t from, direction, to; } TrailLink;
typedef struct { TrailLink links[TRAIL_CAPACITY]; unsigned count; } Trail;

void trail_record(Trail *trail, unsigned from, unsigned direction, unsigned to);
unsigned trail_destination(const Trail *trail, unsigned from, unsigned direction);

#endif
