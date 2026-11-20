#include "trail.h"

unsigned trail_destination(const Trail *trail, unsigned from, unsigned direction) {
    for (unsigned i = trail->count; i > 0; i--) {
        const TrailLink *link = &trail->links[i - 1];
        if (link->from == from && link->direction == direction) return link->to;
    }
    return 0;
}

void trail_record(Trail *trail, unsigned from, unsigned direction, unsigned to) {
    if (!from || !to || from == to || from > UINT8_MAX || to > UINT8_MAX || direction >= TRAIL_DIRECTIONS) return;

    unsigned i = 0;
    while (i < trail->count) {
        const TrailLink *link = &trail->links[i];
        if (link->from == from && link->direction == direction) break;
        i++;
    }
    if (i == trail->count && trail->count == TRAIL_CAPACITY) i = 0;
    if (i < trail->count) {
        for (; i + 1 < trail->count; i++) trail->links[i] = trail->links[i + 1];
    } else trail->count++;
    trail->links[i] = (TrailLink){from, direction, to};
}
