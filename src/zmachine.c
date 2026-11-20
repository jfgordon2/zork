#include "zmachine.h"

enum { MAX_STORY = 96 * 1024, MAX_STREAMS = 8 };
static uint8_t memory[MAX_STORY] __attribute__((aligned(4)));
static const uint8_t *original;
static size_t original_size;
static const ZPlatform *platform;
static unsigned objects;
static bool screen_enabled;
static uint16_t streams[MAX_STREAMS];
static unsigned depth;

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

static void fatal(const char *reason, ...) __attribute__((noreturn));
static void fatal(const char *reason, ...) {
    unsigned pc = GState->logical_pc;
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
        platform->text(text, size);
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
