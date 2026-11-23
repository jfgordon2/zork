#include "zmachine.h"

enum { MAX_STORY = 96 * 1024, MAX_STREAMS = 8 };
static uint8_t memory[MAX_STORY] __attribute__((aligned(4)));
static const uint8_t *original;
static size_t original_size;
static unsigned story_generation;
static const ZPlatform *platform;
static unsigned objects;
static bool screen_enabled;
static uint16_t streams[MAX_STREAMS];
static unsigned depth;
// Shared scratch for a saved game being restored and for z_probe's rewind point;
// the two never overlap.
static uint8_t scratch_dynamic[Z_DYNAMIC_MAX], scratch_stack[Z_STACK_WORDS * 2];
static ZResume probe_batch_state;
static bool probe_batch_active;
static char *probe_reply;
static size_t probe_size, probe_capacity;

void *memset(void *dst, int value, size_t count) {
    unsigned char *bytes = dst;
    for (size_t i = 0; i < count; i++) bytes[i] = (unsigned char)value;
    return dst;
}

void *memcpy(void *dst, const void *src, size_t count) {
    unsigned char *out = dst;
    const unsigned char *in = src;
    for (size_t i = 0; i < count; i++) out[i] = in[i];
    return dst;
}

static uint16_t read_word(const uint8_t *p) { return (uint16_t)(p[0] << 8 | p[1]); }
static void write_word(uint8_t *p, unsigned word) { p[0] = word >> 8; p[1] = word; }

/* Z-machine arithmetic is on 16-bit operands. The GBA has no hardware divide. */
static unsigned divide_word(unsigned numerator, unsigned denominator, unsigned *remainder) {
    unsigned quotient = 0;
    for (unsigned bit = 1u << 15; bit; bit >>= 1) {
        unsigned shifted = denominator * bit;
        if (numerator >= shifted) {
            numerator -= shifted;
            quotient |= bit;
        }
    }
    *remainder = numerator;
    return quotient;
}

static unsigned port_unsigned_remainder(unsigned n, unsigned d) {
    unsigned remainder;
    divide_word(n, d, &remainder);
    return remainder;
}

static int port_signed_divide(int16_t n, int16_t d, bool mod) {
    unsigned remainder;
    unsigned result = divide_word(n < 0 ? -n : n, d < 0 ? -d : d, &remainder);
    if (mod) return n < 0 ? -(int)remainder : (int)remainder;
    return (n < 0) != (d < 0) ? -(int)result : (int)result;
}

static void port_print_number(int value);
static bool port_print_missing_object(unsigned address);
static uint32_t port_entropy(void);
static void port_screen_enabled(bool enabled);
static void port_open_stream(uint16_t address);
static void port_close_stream(void);
static void port_restart(void);
static bool port_save(void);
static bool port_restore(void);

#include "../vendor/mojozork/mojozork.c"

static ZMachineState vm;

static const char *probe_last_line;
const char *z_probe_last_line(void) { return probe_last_line; }

// A probe runs story code speculatively, so a story bug triggered by a probe
// must not end the game. While a probe is active, fatal() unwinds back into
// z_probe(), which rewinds the interpreter and reports the probe as unsuccessful.
// This small context save/restore is written per supported ABI because the GBA
// build is freestanding.
#if defined(__aarch64__)
typedef struct { uint64_t x[12]; uint64_t sp; } ProbeJump;
static ProbeJump probe_jump;
static bool probe_guarded;

// Naked: no prologue, so the saved SP is the caller's and longjmp resumes a
// correct frame. Arguments arrive in x0. returns_twice is essential: it tells
// the optimizer that this call can resume after a later fatal().
__attribute__((naked, noinline, returns_twice))
static int probe_setjmp(ProbeJump *j __attribute__((unused))) {
    __asm__ volatile(
        "stp x19, x20, [x0, #0]\n\t"
        "stp x21, x22, [x0, #16]\n\t"
        "stp x23, x24, [x0, #32]\n\t"
        "stp x25, x26, [x0, #48]\n\t"
        "stp x27, x28, [x0, #64]\n\t"
        "stp x29, x30, [x0, #80]\n\t"
        "mov x9, sp\n\t"
        "str x9, [x0, #96]\n\t"
        "mov x0, #0\n\t"
        "ret\n\t");
}

__attribute__((naked, noinline, noreturn))
static void probe_longjmp(ProbeJump *j __attribute__((unused))) {
    __asm__ volatile(
        "ldp x19, x20, [x0, #0]\n\t"
        "ldp x21, x22, [x0, #16]\n\t"
        "ldp x23, x24, [x0, #32]\n\t"
        "ldp x25, x26, [x0, #48]\n\t"
        "ldp x27, x28, [x0, #64]\n\t"
        "ldp x29, x30, [x0, #80]\n\t"
        "ldr x9, [x0, #96]\n\t"
        "mov sp, x9\n\t"
        "mov x0, #1\n\t"
        "br x30\n\t");
}
#elif defined(__arm__)
typedef struct { uint32_t r[10]; } ProbeJump; // r4-r11, sp, lr
static ProbeJump probe_jump;
static bool probe_guarded;

__attribute__((naked, noinline, returns_twice))
static int probe_setjmp(ProbeJump *j __attribute__((unused))) {
    __asm__ volatile(
        "stmia r0, {r4-r11}\n\t"
        "str sp, [r0, #32]\n\t"
        "str lr, [r0, #36]\n\t"
        "mov r0, #0\n\t"
        "bx lr\n\t");
}

__attribute__((naked, noinline, noreturn))
static void probe_longjmp(ProbeJump *j __attribute__((unused))) {
    __asm__ volatile(
        "ldmia r0, {r4-r11}\n\t"
        "ldr sp, [r0, #32]\n\t"
        "ldr lr, [r0, #36]\n\t"
        "mov r0, #1\n\t"
        "bx lr\n\t");
}
#elif defined(__x86_64__) && !defined(_WIN32)
// SysV x86-64 host builds (including macOS and Linux). Save the callee-saved
// registers and the entry stack pointer; ret from longjmp resumes the call
// site with a nonzero return value, just like setjmp/longjmp.
typedef struct { uint64_t rbx, rbp, r12, r13, r14, r15, rsp; } ProbeJump;
static ProbeJump probe_jump;
static bool probe_guarded;

__attribute__((naked, noinline, returns_twice))
static int probe_setjmp(ProbeJump *j __attribute__((unused))) {
    __asm__ volatile(
        "movq %rbx, 0(%rdi)\n\t"
        "movq %rbp, 8(%rdi)\n\t"
        "movq %r12, 16(%rdi)\n\t"
        "movq %r13, 24(%rdi)\n\t"
        "movq %r14, 32(%rdi)\n\t"
        "movq %r15, 40(%rdi)\n\t"
        "movq %rsp, 48(%rdi)\n\t"
        "xorl %eax, %eax\n\t"
        "ret\n\t");
}

__attribute__((naked, noinline, noreturn))
static void probe_longjmp(ProbeJump *j __attribute__((unused))) {
    __asm__ volatile(
        "movq 0(%rdi), %rbx\n\t"
        "movq 8(%rdi), %rbp\n\t"
        "movq 16(%rdi), %r12\n\t"
        "movq 24(%rdi), %r13\n\t"
        "movq 32(%rdi), %r14\n\t"
        "movq 40(%rdi), %r15\n\t"
        "movq 48(%rdi), %rsp\n\t"
        "movl $1, %eax\n\t"
        "ret\n\t");
}
#else
#error "Probe fatal recovery is not implemented for this ABI"
#endif

static void fatal(const char *reason, ...) __attribute__((noreturn));
static void fatal(const char *reason, ...) {
    unsigned pc = GState->logical_pc;
#if defined(__aarch64__) || defined(__arm__) || (defined(__x86_64__) && !defined(_WIN32))
    if (probe_guarded) {
        probe_guarded = false;
        probe_longjmp(&probe_jump);
    }
#endif
    platform->fatal(reason, pc, pc < original_size ? memory[pc] : 0);
    for (;;) {}
}

static void output(const char *text, size_t size) {
    if (depth) {
        unsigned address = streams[depth - 1];
        unsigned used = read_word(memory + address);
        if (address + 2u + used + size > vm.header.staticmem_addr) fatal("String stream overrun");
        for (size_t i = 0; i < size; i++) memory[address + 2u + used + i] = text[i] == '\n' ? 13 : text[i];
        write_word(memory + address, used + size);
    } else if (screen_enabled && !vm.current_window) {
        if (!probe_reply) platform->text(text, size);
        else for (size_t i = 0; i < size && probe_size < probe_capacity; i++) probe_reply[probe_size++] = text[i];
    }
}

static bool port_print_missing_object(unsigned address) {
    // Release 119's bare SWIM can print the absent direct object in its refusal.
    static const char serial[] = "880429";
    if (read_word(original + 2) != 119 || address != 0x8f88) return false;
    for (unsigned i = 0; i < sizeof(serial) - 1; i++)
        if (original[18 + i] != serial[i]) return false;
    output("dungeon", 7);
    return true;
}

static void port_print_number(int value) {
    char digits[7];
    unsigned offset = sizeof(digits), n = value < 0 ? -value : value;
    do {
        unsigned remainder;
        n = divide_word(n, 10, &remainder);
        digits[--offset] = '0' + remainder;
    } while (n);
    if (value < 0) digits[--offset] = '-';
    output(digits + offset, sizeof(digits) - offset);
}

static uint32_t port_entropy(void) { return platform->entropy(); }
static void port_screen_enabled(bool enabled) { screen_enabled = enabled; }
static void port_open_stream(uint16_t address) {
    if (depth == MAX_STREAMS || address + 2u > vm.header.staticmem_addr) fatal("Invalid string stream");
    streams[depth++] = address;
    write_word(memory + address, 0);
}
static void port_close_stream(void) { if (depth) depth--; }
static bool port_save(void) { return platform->save && platform->save(); }
static bool port_restore(void) { return platform->restore && platform->restore(); }

static void port_restart(void) {
    memcpy(memory, original, original_size);
    random_seed = port_entropy();
    initStory(memory, original_size);
    screen_enabled = true;
    depth = 0;
}

void z_start(const uint8_t *story, size_t size, const ZPlatform *io) {
    GState = &vm;
    platform = io;
    original = story;
    original_size = size;
    story_generation++;
    if (!story_generation) story_generation = 1;
    vm.writestr = output;
    vm.die = fatal;
    if (size < 64 || size > sizeof(memory) || story[0] != 3 || read_word(story + 14) > Z_DYNAMIC_MAX ||
        read_word(story + 26) * 2u > size) fatal("Invalid Zork story file");
    port_restart();
    unsigned first_property = vm.header.staticmem_addr;
    unsigned table = vm.header.objtab_addr + 62;
    objects = 0;
    for (unsigned row = table; row + 9 <= first_property && objects < 255; row += 9) {
        unsigned prop = read_word(memory + row + 7);
        if (prop < row + 9) break;
        if (prop < first_property) first_property = prop;
        objects++;
    }
}

ZResult z_run(unsigned budget) {
    while (budget-- && !vm.waiting_for_line && !vm.quit) {
        if ((size_t)(vm.pc - memory) >= original_size) fatal("Program counter outside story");
        runInstruction();
    }
    return vm.waiting_for_line ? Z_PROMPT : vm.quit ? Z_ENDED : Z_BUSY;
}

unsigned z_input_limit(void) {
    return vm.waiting_for_line && memory[vm.operands[0]] > 1 ? memory[vm.operands[0]] - 1 : 0;
}

bool z_submit(const char *line) {
    unsigned max = z_input_limit();
    if (!max || !*line) return false;
    unsigned size = 0;
    while (line[size] && size <= max) size++;
    if (size > max || vm.operands[0] + size + 2u > vm.header.staticmem_addr) return false;
    uint8_t *text = memory + vm.operands[0] + 1;
    for (unsigned i = 0; i < size; i++) text[i] = line[i] >= 'A' && line[i] <= 'Z' ? line[i] + ('a' - 'A') : line[i];
    text[size] = 0;
    tokenizeUserInput();
    vm.waiting_for_line = vm.step_completed = 0;
    return true;
}

uint16_t z_global(unsigned number) { return number < 240 ? read_word(memory + vm.header.globals_addr + 2 * number) : 0; }
unsigned z_object_count(void) { return objects; }
static bool valid_object(unsigned id) { return id && id <= objects; }
uint8_t z_parent(unsigned id) { return valid_object(id) ? getObjectPtr(id)[4] : 0; }
uint8_t z_sibling(unsigned id) { return valid_object(id) ? getObjectPtr(id)[5] : 0; }
uint8_t z_child(unsigned id) { return valid_object(id) ? getObjectPtr(id)[6] : 0; }

bool z_attr(unsigned id, unsigned attr) {
    return valid_object(id) && attr < 32 && (getObjectPtr(id)[attr >> 3] & (0x80 >> (attr & 7)));
}

size_t z_property(unsigned id, unsigned property, const uint8_t **bytes) {
    uint8_t count = 0;
    *bytes = valid_object(id) ? getObjectProperty(id, property, &count) : NULL;
    return *bytes ? count : 0;
}

void z_name(unsigned id, char *text, size_t capacity) {
    size_t size = capacity ? capacity - 1 : 0;
    if (!capacity) return;
    if (!valid_object(id)) size = 0;
    else {
        const uint8_t *name = getObjectShortName(id);
        if (name[-1]) decode_zscii(name, 0, text, &size);
        else size = 0;
    }
    text[size < capacity ? size : capacity - 1] = 0;
}

static const uint8_t *dictionary(unsigned *stride, unsigned *entries) {
    const uint8_t *table = memory + vm.header.dict_addr;
    const uint8_t *head = table + 1 + table[0];
    *stride = head[0];
    int count = (int16_t)read_word(head + 1);
    *entries = count < 0 ? -count : count;
    return head + 3;
}

unsigned z_dictionary_size(void) {
    unsigned stride, entries;
    dictionary(&stride, &entries);
    return entries;
}

unsigned z_story_generation(void) { return story_generation; }

bool z_dictionary_phrase_known(const char *phrase) {
    unsigned stride, entries;
    const uint8_t *words = dictionary(&stride, &entries);
    const uint8_t *separators = memory + vm.header.dict_addr;
    unsigned separator_count = *separators++;
    bool any = false;
    while (*phrase) {
        if (*phrase == ' ') {
            phrase++;
            continue;
        }
        const char *start = phrase;
        bool separator = is_dictionary_separator((uint8_t)*phrase, separators, separator_count);
        phrase++;
        if (!separator) {
            while (*phrase && *phrase != ' ' &&
                   !is_dictionary_separator((uint8_t)*phrase, separators, separator_count)) phrase++;
        }
        size_t length = (size_t)(phrase - start);
        if (length >= Z_NAME_MAX) length = Z_NAME_MAX - 1;
        char token[Z_NAME_MAX];
        for (size_t i = 0; i < length; i++) {
            char ch = start[i];
            token[i] = ch >= 'A' && ch <= 'Z' ? ch + ('a' - 'A') : ch;
        }
        token[length] = 0;
        if (!dictionary_word((const uint8_t *)token, (unsigned)length, words, stride, entries)) return false;
        any = true;
    }
    return any;
}

void z_dictionary_word(unsigned index, char word[Z_WORD_MAX]) {
    unsigned stride, entries;
    const uint8_t *table = dictionary(&stride, &entries);
    word[0] = 0;
    if (index >= entries || stride < 4) return;
    const uint8_t *entry = table + stride * index;
    if (!(entry[2] & 0x80)) return;
    size_t length = Z_WORD_MAX - 1;
    decode_zscii(entry, 0, word, &length);
    word[length < Z_WORD_MAX ? length : Z_WORD_MAX - 1] = 0;
}

void z_scratch(uint8_t **dynamic, uint8_t **stack) {
    *dynamic = scratch_dynamic;
    *stack = scratch_stack;
}

void z_probe_batch_begin(void) {
    if (probe_batch_active || !vm.waiting_for_line) return;
    probe_batch_state = z_resume();
    memcpy(scratch_dynamic, memory, vm.header.staticmem_addr);
    memcpy(scratch_stack, vm.stack, probe_batch_state.sp * 2u);
    probe_batch_active = true;
}

void z_probe_batch_end(void) {
    probe_batch_active = false;
}

static void effect_hash_word(ZProbeEffect *effect, uint32_t word) {
    for (unsigned shift = 0; shift < 32; shift += 8) {
        uint8_t byte = (uint8_t)(word >> shift);
        effect->first = (effect->first ^ byte) * 16777619u;
        effect->second ^= byte + 0x9e3779b9u + (effect->second << 6) + (effect->second >> 2);
    }
}

static bool address_in_range(unsigned address, unsigned start, unsigned length) {
    return address >= start && address - start < length;
}

static bool probe_transient_address(unsigned address, const ZResume *state) {
    unsigned dynamic_size = vm.header.staticmem_addr;
    if (state->input < dynamic_size) {
        unsigned input_length = 2u + scratch_dynamic[state->input];
        if (input_length > dynamic_size - state->input) input_length = dynamic_size - state->input;
        if (address_in_range(address, state->input, input_length)) return true;
    }
    if (state->parse < dynamic_size) {
        unsigned parse_length = 2u + (unsigned)scratch_dynamic[state->parse] * 4u;
        if (parse_length > dynamic_size - state->parse) parse_length = dynamic_size - state->parse;
        if (address_in_range(address, state->parse, parse_length)) return true;
    }
    return false;
}

// The stock Zork I release 119 build leaves the parser's current action and
// syntax-table globals in slots 15 and 59. Their values depend on which
// accepted synonym was entered (for example MOVE/PUSH in RUG-FCN), even after
// the follow-up command has refreshed the parser tables. Keep those two
// parser-only words out of the persistent-story fingerprint. Other globals
// remain part of the effect, including all puzzle and clock state.
static bool probe_parser_global(unsigned global) {
    return read_word(memory + 2) == 119 &&
        memory[0x12] == '8' && memory[0x13] == '8' && memory[0x14] == '0' &&
        memory[0x15] == '4' && memory[0x16] == '2' && memory[0x17] == '9' &&
        (global == 15 || global == 59);
}

static ZProbeEffect probe_effect_after_run(const ZResume *state) {
    ZProbeEffect effect = {2166136261u, 0x9e3779b9u, true};
    unsigned dynamic_size = vm.header.staticmem_addr;
    for (unsigned address = 0; address < dynamic_size; address++) {
        if (probe_transient_address(address, state) || memory[address] == scratch_dynamic[address]) continue;
        if (address >= vm.header.globals_addr && address - vm.header.globals_addr < 480u &&
            probe_parser_global((address - vm.header.globals_addr) / 2)) continue;
        effect_hash_word(&effect, address);
        effect_hash_word(&effect, memory[address]);
    }
    if (random_seed != state->random) {
        effect_hash_word(&effect, 0xffffffffu);
        effect_hash_word(&effect, random_seed);
    }
    return effect;
}

static bool z_probe_run(const char *line, char *reply, size_t capacity, unsigned budget,
                        ZProbeEffect *effect) {
    if (effect) *effect = (ZProbeEffect){0, 0, false};
    if (!capacity) return false;
    reply[0] = 0;
    if (!vm.waiting_for_line) return false;
    ZResume state = probe_batch_active ? probe_batch_state : z_resume();
    if (!probe_batch_active) {
        memcpy(scratch_dynamic, memory, vm.header.staticmem_addr);
        memcpy(scratch_stack, vm.stack, state.sp * 2u);
    }
    probe_reply = reply;
    probe_size = 0;
    probe_capacity = capacity - 1;
    probe_last_line = line;
    volatile bool answered = false;
#if defined(__aarch64__) || defined(__arm__) || (defined(__x86_64__) && !defined(_WIN32))
    probe_guarded = true;
    if (probe_setjmp(&probe_jump) == 0) {
        answered = z_submit(line) && z_run(budget) == Z_PROMPT;
    } else {
        // A fatal() during the probe unwinds here; discard any partial output.
        probe_size = 0;
    }
    probe_guarded = false;
#else
    answered = z_submit(line) && z_run(budget) == Z_PROMPT;
#endif
    if (answered) {
        // Each candidate verb leaves parser work tables set to its own
        // tokenization. Run a canonical command in the scratch branch so those
        // tables settle before fingerprinting persistent story effects. The
        // command is rewound with the probe, and its output is not captured.
        size_t captured_size = probe_size;
        const char *action_line = probe_last_line;
        probe_last_line = "inventory";
        probe_capacity = captured_size;
        bool normalized = z_submit("inventory") && z_run(budget) == Z_PROMPT;
        probe_last_line = action_line;
        probe_size = captured_size;
        if (!normalized) answered = false;
    }
    if (!answered) probe_size = 0;
    if (answered && effect) *effect = probe_effect_after_run(&state);
    probe_last_line = NULL;
    reply[probe_size < probe_capacity ? probe_size : probe_capacity] = 0;
    probe_reply = NULL;
    z_load(&state, scratch_dynamic, scratch_stack);
    return answered;
}

bool z_probe(const char *line, char *reply, size_t capacity, unsigned budget) {
    return z_probe_run(line, reply, capacity, budget, NULL);
}

bool z_probe_with_effect(const char *line, char *reply, size_t capacity,
                         unsigned budget, ZProbeEffect *effect) {
    return z_probe_run(line, reply, capacity, budget, effect);
}

const uint8_t *z_dynamic(void) { return memory; }
const uint8_t *z_original(void) { return original; }
unsigned z_dynamic_size(void) { return vm.header.staticmem_addr; }
const uint8_t *z_stack_bytes(void) { return (const uint8_t *)vm.stack; }

ZResume z_resume(void) {
    return (ZResume){vm.pc - memory, random_seed, vm.sp - vm.stack, vm.bp,
                     vm.operands[0], vm.operands[1], vm.waiting_for_line != 0};
}

bool z_valid_resume(const ZResume *s) {
    return s->pc < original_size && s->sp <= Z_STACK_WORDS && s->bp <= s->sp &&
        (!s->waiting || (s->input >= 64 && s->input + 2u < vm.header.staticmem_addr &&
                         s->parse >= 64 && s->parse + 2u < vm.header.staticmem_addr));
}

void z_load(const ZResume *s, const uint8_t *dynamic, const uint8_t *stack) {
    memcpy(memory, dynamic, vm.header.staticmem_addr);
    memcpy(vm.stack, stack, s->sp * 2u);
    vm.pc = memory + s->pc;
    vm.logical_pc = s->pc;
    vm.sp = vm.stack + s->sp;
    vm.bp = s->bp;
    vm.operands[0] = s->input;
    vm.operands[1] = s->parse;
    vm.waiting_for_line = vm.step_completed = s->waiting;
    vm.current_window = vm.upper_window_line_count = 0;
    vm.quit = 0;
    random_seed = s->random;
    screen_enabled = true;
    depth = 0;
    if (!s->waiting) doBranch(1);
}
