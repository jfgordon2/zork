#ifndef ZORK_SAVE_H
#define ZORK_SAVE_H

#include <stdbool.h>
#include <stdint.h>
#include "puzzle_map.h"
#include "trail.h"

enum { CHAPTERS = 3, VISITED_BYTES = 32, SRAM_SIZE = 32768 };

bool save_exists(const volatile uint8_t *sram, unsigned chapter);
bool save_write(volatile uint8_t *sram, unsigned chapter, const uint8_t seen[VISITED_BYTES],
                const Trail *trail, const PuzzleMap *puzzle_map);
bool save_read(const volatile uint8_t *sram, unsigned chapter, uint8_t seen[VISITED_BYTES],
               Trail *trail, PuzzleMap *puzzle_map);
bool save_write_with_transcript(volatile uint8_t *sram, unsigned chapter, const uint8_t seen[VISITED_BYTES],
                                const Trail *trail, const PuzzleMap *puzzle_map,
                                /* Compact token stream with NUL-delimited entries. */
                                const uint8_t *transcript, unsigned transcript_size, bool transcript_trimmed);
bool save_read_with_transcript(const volatile uint8_t *sram, unsigned chapter, uint8_t seen[VISITED_BYTES],
                               Trail *trail, PuzzleMap *puzzle_map, uint8_t *transcript,
                               /* Receives the compact token stream with NUL-delimited entries. */
                               unsigned transcript_capacity, unsigned *transcript_size, bool *transcript_trimmed);

#endif
