#include "save.h"
#include "zmachine.h"
#include "visual_state.h"

enum {
    BANK_SIZE = 5120, HEADER_SIZE = 16, RESUME_SIZE = 18,
    TRAIL_HEADER_SIZE = 8, TRAIL_MAGIC = 0x504d,
    TRANSCRIPT_HEADER_SIZE = 10, TRANSCRIPT_MAGIC = 0x4c47, TRANSCRIPT_ESCAPE = 0x1b
};
static const uint32_t signature = 0x31524f5a;  /* ZOR1 */
static uint8_t payload[BANK_SIZE - HEADER_SIZE];
static uint8_t dynamic[Z_DYNAMIC_MAX];
static uint8_t stack[Z_STACK_WORDS * 2];

static uint32_t get(const volatile uint8_t *p, unsigned bytes) {
    uint32_t value = 0;
    for (unsigned i = 0; i < bytes; i++) value |= (uint32_t)p[i] << (8 * i);
    return value;
}

static void put(volatile uint8_t *p, uint32_t value, unsigned bytes) {
    for (unsigned i = 0; i < bytes; i++) p[i] = value >> (8 * i);
}

static uint32_t hash(const volatile uint8_t *data, unsigned size) {
    uint32_t value = 2166136261u;
    for (unsigned i = 0; i < size; i++) value = (value ^ data[i]) * 16777619u;
    return value;
}

static unsigned previous_transcript_entry(const uint8_t *text, unsigned cursor) {
    unsigned end = cursor;
    if (cursor && text[cursor - 1] == 0) end--;
    unsigned start = end;
    while (start && text[start - 1] != 0) start--;
    return start;
}

// Find a valid encoded boundary near the start of a large entry, preferring a line break.
static unsigned transcript_tail_start(const uint8_t *text, unsigned start, unsigned end, unsigned max_bytes) {
    unsigned target = end > max_bytes ? end - max_bytes : start;
    unsigned pos = start;
    while (pos < target && pos < end) {
        pos += text[pos] == TRANSCRIPT_ESCAPE && pos + 1 < end ? 2 : 1;
    }
    unsigned boundary = pos;
    while (pos < end) {
        unsigned next = pos + (text[pos] == TRANSCRIPT_ESCAPE && pos + 1 < end ? 2 : 1);
        if (text[pos] == '\n' ||
            (text[pos] == TRANSCRIPT_ESCAPE && pos + 1 < end && text[pos + 1] == '\n')) {
            boundary = next;
            break;
        }
        pos = next;
    }
    return boundary;
}

static void write_trail(volatile uint8_t *slot, unsigned end, uint32_t sequence, const Trail *trail,
                        const uint8_t *transcript, unsigned transcript_size, bool transcript_trimmed) {
    unsigned remaining = BANK_SIZE - HEADER_SIZE - end;
    if (remaining < TRAIL_HEADER_SIZE) return;

    unsigned transcript_start = 0, transcript_length = 0;
    bool write_transcript = false;
    unsigned route_and_log_space = remaining - TRAIL_HEADER_SIZE;
    if (transcript && transcript_size && route_and_log_space > TRANSCRIPT_HEADER_SIZE) {
        unsigned route_reserve = trail ? route_and_log_space / 4 : 0;
        if (trail && route_reserve > trail->count * 3) route_reserve = trail->count * 3;
        unsigned max_text = route_and_log_space - TRANSCRIPT_HEADER_SIZE;
        if (max_text > route_reserve) max_text -= route_reserve;
        else max_text = 0;
        unsigned cursor = transcript_size, used = 0;
        while (cursor && max_text) {
            unsigned entry_start = previous_transcript_entry(transcript, cursor);
            unsigned entry_size = cursor - entry_start;
            if (used + entry_size > max_text) break;
            used += entry_size;
            transcript_start = entry_start;
            cursor = entry_start;
        }
        if (!used && max_text > 1 && cursor) {
            unsigned entry_start = previous_transcript_entry(transcript, cursor);
            unsigned content_end = cursor && transcript[cursor - 1] == 0 ? cursor - 1 : cursor;
            unsigned start = transcript_tail_start(transcript, entry_start, content_end, max_text - 1);
            if (start < content_end) {
                transcript_start = start;
                transcript_length = transcript_size - start;
                used = transcript_length;
            }
        }
        if (used) {
            if (!transcript_length) transcript_length = transcript_size - transcript_start;
            write_transcript = transcript_length != 0;
        }
    }

    unsigned transcript_storage = write_transcript ? TRANSCRIPT_HEADER_SIZE + transcript_length : 0;
    unsigned count = (remaining - TRAIL_HEADER_SIZE - transcript_storage) / 3;
    if (!trail) count = 0;
    else if (count > trail->count) count = trail->count;
    volatile uint8_t *header = slot + HEADER_SIZE + end;
    volatile uint8_t *data = header + TRAIL_HEADER_SIZE;
    for (unsigned i = 0; i < count; i++) {
        const TrailLink *link = &trail->links[trail->count - count + i];
        *data++ = link->from;
        *data++ = link->direction;
        *data++ = link->to;
    }
    put(header, TRAIL_MAGIC, 2);
    put(header + 2, count, 2);
    put(header + 4, hash(header + TRAIL_HEADER_SIZE, count * 3) ^ sequence, 4);

    if (write_transcript) {
        volatile uint8_t *log_header = header + TRAIL_HEADER_SIZE + count * 3;
        volatile uint8_t *log_data = log_header + TRANSCRIPT_HEADER_SIZE;
        for (unsigned i = 0; i < transcript_length; i++) log_data[i] = transcript[transcript_start + i];
        put(log_header + 2, transcript_length, 2);
        put(log_header + 4, ((transcript_trimmed || transcript_start != 0) ? 1 : 0) | 4, 2);
        put(log_header + 6, hash(log_data, transcript_length) ^ sequence, 4);
        put(log_header, TRANSCRIPT_MAGIC, 2);
    }
}

static void read_trail(const volatile uint8_t *slot, unsigned end, Trail *trail, uint8_t *transcript,
                       unsigned transcript_capacity, unsigned *transcript_size, bool *transcript_trimmed) {
    if (trail) trail->count = 0;
    if (transcript_size) *transcript_size = 0;
    if (transcript_trimmed) *transcript_trimmed = false;
    unsigned remaining = BANK_SIZE - HEADER_SIZE - end;
    if (remaining < TRAIL_HEADER_SIZE) return;
    const volatile uint8_t *header = slot + HEADER_SIZE + end;
    if (get(header, 2) != TRAIL_MAGIC) return;
    unsigned count = get(header + 2, 2);
    const volatile uint8_t *data = header + TRAIL_HEADER_SIZE;
    if (count > TRAIL_CAPACITY || count * 3 > remaining - TRAIL_HEADER_SIZE ||
        (hash(data, count * 3) ^ get(slot + 4, 4)) != get(header + 4, 4)) return;
    if (trail) {
        for (unsigned i = 0; i < count; i++, data += 3) trail_record(trail, data[0], data[1], data[2]);
    }

    unsigned used = TRAIL_HEADER_SIZE + count * 3;
    if (!transcript_size || remaining < used + TRANSCRIPT_HEADER_SIZE) return;
    const volatile uint8_t *log_header = header + used;
    if (get(log_header, 2) != TRANSCRIPT_MAGIC) return;
    unsigned length = get(log_header + 2, 2);
    const volatile uint8_t *log_data = log_header + TRANSCRIPT_HEADER_SIZE;
    if (length > remaining - used - TRANSCRIPT_HEADER_SIZE ||
        (hash(log_data, length) ^ get(slot + 4, 4)) != get(log_header + 6, 4)) return;
    unsigned flags = get(log_header + 4, 2);
    if (!(flags & 4)) return;
    unsigned copy = transcript ? length : 0;
    if (transcript) {
        if (copy > transcript_capacity) return;
        for (unsigned i = 0; i < copy; i++) transcript[i] = log_data[i];
        if (copy < transcript_capacity) transcript[copy] = 0;
    }
    *transcript_size = copy;
    if (transcript_trimmed) *transcript_trimmed = (flags & 1) != 0;
}

static unsigned bank_number(const volatile uint8_t *sram, unsigned chapter) {
    const volatile uint8_t *first = sram + chapter * 2 * BANK_SIZE;
    bool valid[2];
    for (unsigned i = 0; i < 2; i++) {
        const volatile uint8_t *p = first + i * BANK_SIZE;
        unsigned n = get(p + 8, 2);
        valid[i] = get(p, 4) == signature && get(p + 10, 2) == chapter &&
            n >= RESUME_SIZE + VISITED_BYTES && n <= sizeof(payload) &&
            (hash(p + HEADER_SIZE, n) ^ get(p + 4, 4)) == get(p + 12, 4);
    }
    if (!valid[0]) return valid[1] ? 1 : 2;
    if (!valid[1]) return 0;
    return (int32_t)(get(first + BANK_SIZE + 4, 4) - get(first + 4, 4)) > 0 ? 1 : 0;
}

bool save_exists(const volatile uint8_t *sram, unsigned chapter) {
    return chapter < CHAPTERS && bank_number(sram, chapter) < 2;
}

static uint8_t difference(const uint8_t *a, const uint8_t *base, unsigned i) {
    return base ? a[i] ^ base[i] : a[i];
}

/* A clear high bit skips unchanged bytes. A set bit introduces literal XOR bytes. */
static bool encode(unsigned *pos, const uint8_t *a, const uint8_t *base, unsigned size) {
    for (unsigned i = 0; i < size;) {
        bool literal = difference(a, base, i) != 0;
        unsigned n = 1;
        while (n < 128 && i + n < size && (difference(a, base, i + n) != 0) == literal) n++;
        if (*pos + 1 + (literal ? n : 0) > sizeof(payload)) return false;
        payload[(*pos)++] = (n - 1) | (literal ? 128 : 0);
        if (literal) for (unsigned j = 0; j < n; j++) payload[(*pos)++] = difference(a, base, i + j);
        i += n;
    }
    return true;
}

static bool decode(unsigned *pos, unsigned end, uint8_t *a, const uint8_t *base, unsigned size) {
    for (unsigned i = 0; i < size;) {
        if (*pos == end) return false;
        unsigned tag = payload[(*pos)++], n = (tag & 127) + 1;
        bool literal = tag & 128;
        if (i + n > size || *pos + (literal ? n : 0) > end) return false;
        for (unsigned j = 0; j < n; j++, i++) {
            a[i] = (base ? base[i] : 0) ^ (literal ? payload[(*pos)++] : 0);
        }
    }
    return true;
}

bool save_write_with_transcript(volatile uint8_t *sram, unsigned chapter, const uint8_t seen[VISITED_BYTES],
                                const Trail *trail, const PuzzleMap *puzzle_map,
                                const uint8_t *transcript, unsigned transcript_size, bool transcript_trimmed) {
    if (chapter >= CHAPTERS) return false;
    ZResume state = z_resume();
    if (!z_valid_resume(&state)) return false;
    put(payload, state.pc, 4);
    put(payload + 4, state.random, 4);
    put(payload + 8, state.sp, 2);
    put(payload + 10, state.bp, 2);
    put(payload + 12, state.input, 2);
    put(payload + 14, state.parse, 2);
    put(payload + 16, state.waiting, 1);
    // Previously reserved byte: presentation only, without changing the story snapshot.
    uint8_t presentation = visual_state_save() &
                           (chapter == 0 ? 0x80 : chapter == 1 ? 0x7f : 0x3f);
    bool save_map = chapter == 2 && puzzle_map && puzzle_map->valid;
    bool save_z3_state = chapter == 2;
    if (save_z3_state) {
        presentation = (visual_state_parapet_dial() - 1) & 7;
        presentation |= (visual_state_vista() & 7) << 3;
        if (save_map) presentation |= 0x40;
        presentation |= 0x80;
    }
    put(payload + 17, presentation, 1);
    for (unsigned i = 0; i < VISITED_BYTES; i++) payload[RESUME_SIZE + i] = seen[i];
    unsigned end = RESUME_SIZE + VISITED_BYTES;
    if (!encode(&end, z_dynamic(), z_original(), z_dynamic_size()) ||
        !encode(&end, z_stack_bytes(), 0, state.sp * 2u)) return false;
    if (save_z3_state) {
        if (end + 4 > sizeof(payload)) return false;
        uint32_t packed = 0;
        if (save_map) {
            for (unsigned i = 0; i < 9; i++) packed |= ((uint32_t)puzzle_map->tiles[i] & 7) << (i * 3);
        }
        packed |= ((uint32_t)visual_state_mirror_arrow() & 7) << 27;
        if (visual_state_mirror_pole_raised()) packed |= 1u << 30;
        if (visual_state_mirror_pine_open()) packed |= 1u << 31;
        for (unsigned i = 0; i < 4; i++) payload[end++] = (uint8_t)(packed >> (i * 8));
    }

    unsigned current = bank_number(sram, chapter);
    volatile uint8_t *slot = sram + chapter * 2 * BANK_SIZE;
    uint32_t sequence = current < 2 ? get(slot + current * BANK_SIZE + 4, 4) + 1 : 1;
    if (current == 0) slot += BANK_SIZE;
    put(slot, 0, 4);
    put(slot + 4, sequence, 4);
    put(slot + 8, end, 2);
    put(slot + 10, chapter, 2);
    put(slot + 12, hash(payload, end) ^ sequence, 4);
    for (unsigned i = 0; i < end; i++) slot[HEADER_SIZE + i] = payload[i];
    write_trail(slot, end, sequence, trail, transcript, transcript_size, transcript_trimmed);
    put(slot, signature, 4);
    return get(slot, 4) == signature &&
        (hash(slot + HEADER_SIZE, end) ^ sequence) == get(slot + 12, 4);
}

bool save_write(volatile uint8_t *sram, unsigned chapter, const uint8_t seen[VISITED_BYTES],
                const Trail *trail, const PuzzleMap *puzzle_map) {
    return save_write_with_transcript(sram, chapter, seen, trail, puzzle_map, 0, 0, false);
}

bool save_read_with_transcript(const volatile uint8_t *sram, unsigned chapter, uint8_t seen[VISITED_BYTES],
                               Trail *trail, PuzzleMap *puzzle_map, uint8_t *transcript,
                               unsigned transcript_capacity, unsigned *transcript_size, bool *transcript_trimmed) {
    if (chapter >= CHAPTERS) return false;
    unsigned index = bank_number(sram, chapter);
    if (index >= 2) return false;
    const volatile uint8_t *slot = sram + (chapter * 2 + index) * BANK_SIZE;
    unsigned end = get(slot + 8, 2);
    for (unsigned i = 0; i < end; i++) payload[i] = slot[HEADER_SIZE + i];
    bool has_z3_state = chapter == 2 && (payload[17] & 0x80) != 0;
    unsigned story_end = end;
    if (has_z3_state) {
        if (story_end < RESUME_SIZE + VISITED_BYTES + 4) return false;
        story_end -= 4;
    }
    ZResume state = {get(payload, 4), get(payload + 4, 4), get(payload + 8, 2), get(payload + 10, 2),
                     get(payload + 12, 2), get(payload + 14, 2), get(payload + 16, 1) != 0};
    unsigned pos = RESUME_SIZE + VISITED_BYTES;
    if (!z_valid_resume(&state) || !decode(&pos, story_end, dynamic, z_original(), z_dynamic_size()) ||
        !decode(&pos, story_end, stack, 0, state.sp * 2u) || pos != story_end) return false;
    for (unsigned i = 0; i < VISITED_BYTES; i++) seen[i] = payload[RESUME_SIZE + i];
    read_trail(slot, end, trail, transcript, transcript_capacity, transcript_size, transcript_trimmed);
    z_load(&state, dynamic, stack);
    uint8_t presentation = chapter == 2 ? payload[17] & 0x38 :
                           chapter == 1 ? payload[17] & 0x7f : payload[17] & 0x80;
    visual_state_restore(presentation);
    uint32_t packed = 0;
    if (has_z3_state) {
        packed = (uint32_t)payload[story_end] |
                 ((uint32_t)payload[story_end + 1] << 8) |
                 ((uint32_t)payload[story_end + 2] << 16) |
                 ((uint32_t)payload[story_end + 3] << 24);
        visual_state_restore_z3((payload[17] & 7) + 1, (packed >> 27) & 7,
                                (packed & (1u << 30)) != 0, (packed & (1u << 31)) != 0);
    }
    if (puzzle_map) {
        puzzle_map->valid = false;
        if (has_z3_state && (payload[17] & 0x40)) {
            for (unsigned i = 0; i < 9; i++) puzzle_map->tiles[i] = (PuzzleTile)((packed >> (i * 3)) & 7);
            puzzle_map->valid = true;
        }
    }
    return true;
}

bool save_read(const volatile uint8_t *sram, unsigned chapter, uint8_t seen[VISITED_BYTES],
               Trail *trail, PuzzleMap *puzzle_map) {
    return save_read_with_transcript(sram, chapter, seen, trail, puzzle_map, 0, 0, 0, 0);
}
