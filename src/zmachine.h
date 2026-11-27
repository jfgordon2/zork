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
bool z_probe_menu(const char *line, char *reply, size_t capacity, unsigned budget);
// Like z_probe, but also returns global 0 as it stood after the command. The
// trilogy stores the current room there, so route audits can distinguish a
// successful transition from a parser reply that merely sounds plausible.
bool z_probe_with_room(const char *line, char *reply, size_t capacity,
                       unsigned budget, unsigned *room);
// Like z_probe_with_room, but also returns the current parent of `object`
// after the speculative command. The snapshot is restored before returning.
bool z_probe_with_room_and_parent(const char *line, char *reply, size_t capacity,
                                  unsigned budget, unsigned object,
                                  unsigned *room, unsigned *parent);
// Instruction-sliced menu probe. Each call runs at most `chunk` Z-machine
// instructions and returns BUSY until the current phase reaches the prompt.
// After the action phase, the caller may request the inventory normalization
// and then complete the probe to restore the batch state.
typedef enum { Z_PROBE_BUSY, Z_PROBE_DONE, Z_PROBE_FAILED } ZProbeStep;
ZProbeStep z_probe_menu_begin(const char *line, char *reply, size_t capacity,
                              unsigned budget, unsigned chunk);
ZProbeStep z_probe_menu_continue(unsigned chunk);
ZProbeStep z_probe_menu_normalize(unsigned chunk);
// Computes and stores the persistent-effect fingerprint without restoring the
// story, so the caller can run it in its own frame.
ZProbeStep z_probe_menu_hash(ZProbeEffect *effect);
bool z_probe_menu_complete(ZProbeEffect *effect);
void z_probe_menu_abort(void);
bool z_probe_with_effect(const char *line, char *reply, size_t capacity,
                         unsigned budget, ZProbeEffect *effect);
#ifdef PROFILE
// Longest input-tokenization time seen since the last menu filter reset.
extern uint16_t z_profile_tokenize_ticks;
extern uint16_t z_profile_hash_ticks;
extern uint16_t z_profile_restore_ticks;
#endif
#ifdef PROFILE_TRACE
// Slowest single story instruction seen since the last menu filter reset.
extern uint16_t z_profile_slow_pc, z_profile_slow_ticks;
extern uint8_t z_profile_slow_opcode;
#endif
const char *z_probe_last_line(void);

#endif
