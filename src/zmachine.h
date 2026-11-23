#ifndef ZMACHINE_H
#define ZMACHINE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum { Z_DYNAMIC_MAX = 12800, Z_STACK_WORDS = 2048, Z_NAME_MAX = 48, Z_WORD_MAX = 10 };
typedef enum { Z_BUSY, Z_PROMPT, Z_ENDED } ZResult;

typedef struct {
    uint32_t pc, random;
    uint16_t sp, bp, input, parse;
    bool waiting;
} ZResume;

typedef struct {
    uint32_t first, second;
    bool valid;
} ZProbeEffect;

typedef struct {
    void (*text)(const char *text, size_t size);
    void (*fatal)(const char *reason, unsigned pc, unsigned opcode);
    uint32_t (*entropy)(void);
    bool (*save)(void);
    bool (*restore)(void);
} ZPlatform;

void z_start(const uint8_t *story, size_t size, const ZPlatform *platform);
ZResult z_run(unsigned budget);
bool z_submit(const char *line);
unsigned z_input_limit(void);
uint16_t z_global(unsigned number);
unsigned z_object_count(void);
uint8_t z_parent(unsigned object);
uint8_t z_child(unsigned object);
uint8_t z_sibling(unsigned object);
bool z_attr(unsigned object, unsigned attribute);
size_t z_property(unsigned object, unsigned property, const uint8_t **bytes);
void z_name(unsigned object, char *text, size_t capacity);
unsigned z_dictionary_size(void);
unsigned z_story_generation(void);
bool z_dictionary_phrase_known(const char *phrase);
void z_dictionary_word(unsigned index, char word[Z_WORD_MAX]);
const uint8_t *z_dynamic(void);
const uint8_t *z_original(void);
unsigned z_dynamic_size(void);
const uint8_t *z_stack_bytes(void);
ZResume z_resume(void);
bool z_valid_resume(const ZResume *state);
void z_load(const ZResume *state, const uint8_t *dynamic, const uint8_t *stack);
// Buffers large enough for a dynamic-memory image and a stack image.
void z_scratch(uint8_t **dynamic, uint8_t **stack);
void z_probe_batch_begin(void);
void z_probe_batch_end(void);
// Runs one command at the prompt with its text captured into reply instead of
// shown, then rewinds the story to exactly where it was. Uses z_scratch.
bool z_probe(const char *line, char *reply, size_t capacity, unsigned budget);
bool z_probe_with_effect(const char *line, char *reply, size_t capacity,
                         unsigned budget, ZProbeEffect *effect);
const char *z_probe_last_line(void);

#endif
