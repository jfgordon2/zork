/**
 * MojoZork; a simple, just-for-fun implementation of Infocom's Z-Machine.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

// The Z-Machine specifications 1.1:
//     https://inform-fiction.org/zmachine/standards/z1point1/index.html

/* Altered for the freestanding Zork Trilogy GBA port.
 * Platform I/O and input suspension are provided by src/zmachine.c.
 */
#define dbg(...) ((void) 0)
#define FIXME(what)

// the "_t" drives me nuts.  :/
typedef uint8_t uint8;
typedef int8_t sint8;
typedef uint16_t uint16;
typedef int16_t sint16;
typedef uint32_t uint32;
typedef int32_t sint32;
typedef uint64_t uint64;
typedef int64_t sint64;

typedef size_t uintptr;

// !!! FIXME: maybe kill these.
#define READUI8(ptr) *(ptr++)
#define READUI16(ptr) ((((uint16) ptr[0]) << 8) | ((uint16) ptr[1])); ptr += sizeof (uint16)
#define WRITEUI16(dst, src) { *(dst++) = (uint8) ((src >> 8) & 0xFF); *(dst++) = (uint8) (src & 0xFF); }

typedef void (*OpcodeFn)(void);

typedef struct
{
    const char *name;
    OpcodeFn fn;
} Opcode;

typedef struct ZHeader
{
    uint8 version;
    uint8 flags1;
    uint16 release;
    uint16 himem_addr;
    uint16 pc_start;  // in ver6, packed address of main()
    uint16 dict_addr;
    uint16 objtab_addr;
    uint16 globals_addr;
    uint16 staticmem_addr;  // offset of static memory, also: size of dynamic mem.
    uint16 flags2;
    char serial_code[7];  // six ASCII chars in ver2. In ver3+: ASCII of completion date: YYMMDD
    uint16 abbrtab_addr;  // abbreviations table
    uint16 story_len;
    uint16 story_checksum;
    // !!! FIXME: more fields here, all of which are ver4+
} ZHeader;


typedef struct ZMachineState
{
    uint32 instructions_run;
    uint8 *story;
    uintptr story_len;
    ZHeader header;
    uint32 logical_pc;
    const uint8 *pc;  // program counter
    uint16 *sp;  // stack pointer
    uint16 bp;  // base pointer
    uint16 calculated_checksum;
    int quit;
    int step_completed;  // possibly time to break out of the Z-Machine simulation loop.
    uint16 stack[2048];  // !!! FIXME: make this dynamic?
    uint16 operands[8];
    uint8 operand_count;
    char alphabet_table[78];
    int waiting_for_line;
    uint16 current_window;
    uint16 upper_window_line_count;  // if 0, there is no window split.

    // this is kinda wasteful (we could pack the 89 opcodes in their various forms
    //  into separate arrays and strip off the metadata bits) but it simplifies
    //  some things to just have a big linear array.
    Opcode opcodes[256];

    // The extended ones, however, only have one form, so we pack that tight.
    Opcode extended_opcodes[30];

    void (*split_window)(const uint16 oldval, const uint16 newval);
    void (*set_window)(const uint16 oldval, const uint16 newval);

    void (*writestr)(const char *str, const uintptr slen);
    #if defined(__GNUC__) || defined(__clang__)
    void (*die)(const char *fmt, ...) __attribute__((noreturn));
    #else
    void (*die)(const char *fmt, ...);
    #endif
} ZMachineState;

static ZMachineState *GState = NULL;


static uint8 *get_virtualized_mem_ptr(const uint16 offset);
static uint16 remap_objectid(const uint16 objid);

#ifndef MULTIZORK
static uint8 *get_virtualized_mem_ptr(const uint16 offset) { return GState->story + offset; }
static uint16 remap_objectid(const uint16 objid) { return objid; }
#endif

// The Z-Machine can't directly address 32-bits, but this needs to expand past 16 bits when we multiply by 2, 4, or 8, etc.
static uint8 *unpackAddress(const uint32 addr)
{
    if (GState->header.version <= 3) {
        return (GState->story + (addr * 2));
    } else if (GState->header.version <= 5) {
        return (GState->story + (addr * 4));
    } else if (GState->header.version <= 6) {
        GState->die("write me");  //   4P + 8R_O    Versions 6 and 7, for routine calls ... or 4P + 8S_O    Versions 6 and 7, for print_paddr
    } else if (GState->header.version <= 8) {
        return (GState->story + (addr * 8));
    }

    GState->die("FIXME Unsupported version for packed addressing");
    return NULL;
}

static uint8 *varAddress(const uint8 var, const int writing, const int indirect)
{
    if (var == 0) { // top of stack
        // "6.3.4: In the seven opcodes that take indirect variable references (inc, dec, inc_chk, dec_chk, load, store, pull), an indirect reference to the stack pointer does not push or pull the top item of the stack - it is read or written in place."
        if (indirect) {
            if (GState->sp == GState->stack) {
                GState->die("Stack underflow");
            }
            return (uint8 *) (GState->sp - 1);
        } else if (writing) {
            if ((size_t)(GState->sp-GState->stack) >= (sizeof (GState->stack) / sizeof (GState->stack[0]))) {
                GState->die("Stack overflow");
            }
            dbg("push stack\n");
            return (uint8 *) GState->sp++;
        } else {
            if (GState->sp == GState->stack) {
                GState->die("Stack underflow");  // nothing on the stack at all?
            }
            const uint16 numlocals = GState->bp ? GState->stack[GState->bp-1] : 0;
            if ((GState->bp + numlocals) >= (GState->sp-GState->stack))
                GState->die("Stack underflow");  // no stack data left in this frame.

            dbg("pop stack\n");
            return (uint8 *) --GState->sp;
        }
    } else if ((var >= 0x1) && (var <= 0xF)) {  // local var.
        if (GState->stack[GState->bp-1] <= (var-1)) {
            GState->die("referenced unallocated local var #%u (%u available)", (unsigned int) (var-1), (unsigned int) GState->stack[GState->bp-1]);
        }
        return (uint8 *) &GState->stack[GState->bp + (var-1)];
    }

    // else, global var
    FIXME("check for overflow, etc");
    return (GState->story + GState->header.globals_addr) + ((var-0x10) * sizeof (uint16));
}

static void opcode_call(void)
{
    uint8 args = GState->operand_count;
    const uint16 *operands = GState->operands;
    const uint8 storeid = *(GState->pc++);
    // no idea if args==0 should be the same as calling addr 0...
    if ((args == 0) || (operands[0] == 0)) {  // legal no-op; store 0 to return value and bounce.
        uint8 *store = varAddress(storeid, 1, 0);
        WRITEUI16(store, 0);
    } else {
        const uint8 *routine = unpackAddress(operands[0]);
        GState->logical_pc = (uint32) (routine - GState->story);
        const uint8 numlocals = *(routine++);
        if (numlocals > 15) {
            GState->die("Routine has too many local variables (%u)", numlocals);
        }

        FIXME("check for stack overflow here");

        *(GState->sp++) = (uint16) storeid;  // save where we should store the call's result.

        // next instruction to run upon return.
        const uint32 pcoffset = (uint32) (GState->pc - GState->story);
        *(GState->sp++) = (pcoffset & 0xFFFF);
        *(GState->sp++) = ((pcoffset >> 16) & 0xFFFF);

        *(GState->sp++) = GState->bp;  // current base pointer before the call.
        *(GState->sp++) = numlocals;  // number of locals we're allocating.

        GState->bp = (uint16) (GState->sp-GState->stack);

        sint8 i;
        if (GState->header.version <= 4) {
            for (i = 0; i < numlocals; i++) {
                uint8 *local = (uint8 *) GState->sp++;
                local[0] = *routine++;
                local[1] = *routine++;
            }
        } else {
            for (i = 0; i < numlocals; i++) {
                *(GState->sp++) = 0;
            }
        }

        args--;  // remove the return address from the count.
        if (args > numlocals) {  // it's legal to have more args than locals, throw away the extras.
            args = numlocals;
        }

        const uint16 *src = operands + 1;
        uint8 *dst = (uint8 *) (GState->stack + GState->bp);
        for (i = 0; i < args; i++) {
            WRITEUI16(dst, src[i]);
        }

        GState->pc = routine;
        // next call to runInstruction() will execute new routine.
    }
} // opcode_call

static void doReturn(const uint16 val)
{
    FIXME("newer versions start in a real routine, but still aren't allowed to return from it.");
    if (GState->bp == 0)
        GState->die("Stack underflow in return operation");

    dbg("popping stack for return\n");
    dbg("returning: initial pc=%X, bp=%u, sp=%u\n", (unsigned int) (GState->pc-GState->story), (unsigned int) GState->bp, (unsigned int) (GState->sp-GState->stack));

    GState->sp = GState->stack + GState->bp;  // this dumps all the locals and data pushed on the stack during the routine.
    GState->sp--;  // dump our copy of numlocals
    GState->bp = *(--GState->sp);  // restore previous frame's base pointer, dump it from the stack.

    GState->sp -= 2;  // point to start of our saved program counter.
    const uint32 pcoffset = ((uint32) GState->sp[0]) | (((uint32) GState->sp[1]) << 16);

    GState->pc = GState->story + pcoffset;  // next instruction is one following our original call.

    const uint8 storeid = (uint8) *(--GState->sp);  // pop the result storage location.

    dbg("returning: new pc=%X, bp=%u, sp=%u\n", (unsigned int) (GState->pc-GState->story), (unsigned int) GState->bp, (unsigned int) (GState->sp-GState->stack));
    uint8 *store = varAddress(storeid, 1, 0);  // and store the routine result.
    WRITEUI16(store, val);
}

static void opcode_ret(void)
{
    doReturn(GState->operands[0]);
}

static void opcode_rtrue(void)
{
    doReturn(1);
}

static void opcode_rfalse(void)
{
    doReturn(0);
}

static void opcode_ret_popped(void)
{
    uint8 *ptr = varAddress(0, 0, 0);   // top of stack.
    const uint16 result = READUI16(ptr);
    doReturn(result);
}

static void opcode_push(void)
{
    uint8 *store = varAddress(0, 1, 0);   // top of stack.
    WRITEUI16(store, GState->operands[0]);
}

static void opcode_pull(void)
{
    const uint8 *ptr = varAddress(0, 0, 0);   // top of stack.
    const uint16 val = READUI16(ptr);
    uint8 *store = varAddress((uint8) GState->operands[0], 1, 1);
    WRITEUI16(store, val);
}

static void opcode_pop(void)
{
    varAddress(0, 0, 0);   // this causes a pop.
}



static void opcode_show_status(void)
{
    // The GBA UI reads the status variables directly.
}

static void opcode_add(void)
{
    uint8 *store = varAddress(*(GState->pc++), 1, 0);
    const sint16 result = ((sint16) GState->operands[0]) + ((sint16) GState->operands[1]);
    WRITEUI16(store, result);
}

static void opcode_sub(void)
{
    uint8 *store = varAddress(*(GState->pc++), 1, 0);
    const sint16 result = ((sint16) GState->operands[0]) - ((sint16) GState->operands[1]);
    WRITEUI16(store, result);
}

static void doBranch(int truth)
{
    const uint8 branch = *(GState->pc++);
    const int farjump = (branch & (1<<6)) == 0;
    const int onTruth = (branch & (1<<7)) ? 1 : 0;

    const uint8 byte2 = farjump ? *(GState->pc++) : 0;

    if (truth == onTruth) {  // take the branch?
        sint16 offset = (sint16) (branch & 0x3F);
        if (farjump) {
            if (offset & (1 << 5)) {
                offset |= 0xC0;   // extend out sign bit.
            }
            offset = (offset << 8) | ((sint16) byte2);
        }

        if (offset == 0) {  // return false from current routine.
            doReturn(0);
        } else if (offset == 1) {  // return true from current routine.
            doReturn(1);
        } else {
            GState->pc = (GState->pc + offset) - 2;  // branch.
        }
    }
}

static void opcode_je(void)
{
    const uint16 a = GState->operands[0];
    sint8 i;
    for (i = 1; i < GState->operand_count; i++) {
        if (a == GState->operands[i]) {
            doBranch(1);
            return;
        }
    }
    doBranch(0);
}

static void opcode_jz(void)
{
    doBranch((GState->operands[0] == 0) ? 1 : 0);
}

static void opcode_jl(void)
{
    doBranch((((sint16) GState->operands[0]) < ((sint16) GState->operands[1])) ? 1 : 0);
}

static void opcode_jg(void)
{
    doBranch((((sint16) GState->operands[0]) > ((sint16) GState->operands[1])) ? 1 : 0);
}

static void opcode_test(void)
{
    doBranch((GState->operands[0] & GState->operands[1]) == GState->operands[1]);
}

static void opcode_jump(void)
{
    // this opcode is not a branch instruction, and doesn't follow those rules.
    FIXME("make sure GState->pc is valid");
    GState->pc = (GState->pc + ((sint16) GState->operands[0])) - 2;
}

static void opcode_div(void)
{
    uint8 *store = varAddress(*(GState->pc++), 1, 0);
    if (GState->operands[1] == 0) {
        GState->die("Division by zero");
    }
    const uint16 result = (uint16) (port_signed_divide(GState->operands[0], GState->operands[1], false));
    WRITEUI16(store, result);
}

static void opcode_mod(void)
{
    uint8 *store = varAddress(*(GState->pc++), 1, 0);
    if (GState->operands[1] == 0) {
        GState->die("Division by zero");
    }
    const uint16 result = (uint16) (port_signed_divide(GState->operands[0], GState->operands[1], true));
    WRITEUI16(store, result);
}

static void opcode_mul(void)
{
    uint8 *store = varAddress(*(GState->pc++), 1, 0);
    const uint16 result = (uint16) (((sint16) GState->operands[0]) * ((sint16) GState->operands[1]));
    WRITEUI16(store, result);
}

static void opcode_or(void)
{
    uint8 *store = varAddress(*(GState->pc++), 1, 0);
    const uint16 result = (GState->operands[0] | GState->operands[1]);
    WRITEUI16(store, result);
}

static void opcode_and(void)
{
    uint8 *store = varAddress(*(GState->pc++), 1, 0);
    const uint16 result = (GState->operands[0] & GState->operands[1]);
    WRITEUI16(store, result);
}

static void opcode_not(void)
{
    uint8 *store = varAddress(*(GState->pc++), 1, 0);
    const uint16 result = ~GState->operands[0];
    WRITEUI16(store, result);
}

static void opcode_inc_chk(void)
{
    uint8 *store = varAddress((uint8) GState->operands[0], 0, 1);
    sint16 val = READUI16(store);
    store -= sizeof (uint16);
    val++;
    WRITEUI16(store, (uint16) val);
    doBranch( (((sint16) val) > ((sint16) GState->operands[1])) ? 1 : 0 );
}

static void opcode_inc(void)
{
    uint8 *store = varAddress((uint8) GState->operands[0], 0, 1);
    sint16 val = (sint16) READUI16(store);
    store -= sizeof (uint16);
    val++;
    WRITEUI16(store, (uint16) val);
}

static void opcode_dec_chk(void)
{
    uint8 *store = varAddress((uint8) GState->operands[0], 0, 1);
    sint16 val = (sint16) READUI16(store);
    store -= sizeof (uint16);
    val--;
    WRITEUI16(store, (uint16) val);
    doBranch( (((sint16) val) < ((sint16) GState->operands[1])) ? 1 : 0 );
}

static void opcode_dec(void)
{
    uint8 *store = varAddress((uint8) GState->operands[0], 0, 1);
    sint16 val = (sint16) READUI16(store);
    store -= sizeof (uint16);
    val--;
    WRITEUI16(store, (uint16) val);
}

static void opcode_load(void)
{
    const uint8 *valptr = varAddress((uint8) (GState->operands[0] & 0xFF), 0, 1);
    const uint16 val = READUI16(valptr);
    uint8 *store = varAddress(*(GState->pc++), 1, 0);
    WRITEUI16(store, val);
}

static void opcode_loadw(void)
{
    uint8 *store = varAddress(*(GState->pc++), 1, 0);
    FIXME("can only read from dynamic or static memory (not highmem).");
    FIXME("how does overflow work here? Do these wrap around?");
    const uint16 offset = (GState->operands[0] + (GState->operands[1] * 2));
    const uint8 *src = get_virtualized_mem_ptr(offset);
    store[0] = src[0];
    store[1] = src[1];
}

static void opcode_loadb(void)
{
    uint8 *store = varAddress(*(GState->pc++), 1, 0);
    FIXME("can only read from dynamic or static memory (not highmem).");
    FIXME("how does overflow work here? Do these wrap around?");
    const uint16 offset = (GState->operands[0] + GState->operands[1]);
    const uint8 *src = get_virtualized_mem_ptr(offset);
    const uint16 value = *src;  // expand out to 16-bit before storing.
    WRITEUI16(store, value);
}

static void opcode_storew(void)
{
    const uint16 offset = (GState->operands[0] + (GState->operands[1] * 2));
    // A legacy story can attempt a write past dynamic memory. Keep its code and text read-only.
    if (offset + 1u >= GState->header.staticmem_addr) return;
    uint8 *dst = get_virtualized_mem_ptr(offset);
    const uint16 src = GState->operands[2];
    WRITEUI16(dst, src);
}

static void opcode_storeb(void)
{
    const uint16 offset = (GState->operands[0] + GState->operands[1]);
    if (offset >= GState->header.staticmem_addr) return;
    uint8 *dst = get_virtualized_mem_ptr(offset);
    const uint8 src = (uint8) GState->operands[2];
    *dst = src;
}

static void opcode_store(void)
{
    uint8 *store = varAddress((uint8) (GState->operands[0] & 0xFF), 1, 1);
    const uint16 src = GState->operands[1];
    WRITEUI16(store, src);
}

static uint8 *getObjectPtr(const uint16 objid);
#ifndef MULTIZORK
static uint8 *getObjectPtr(const uint16 objid)
{
    if (objid == 0) {
        GState->die("Object id #0 referenced");
    }

    if ((GState->header.version <= 3) && (objid > 255)) {
        GState->die("Invalid object id referenced");
    }

    uint8 *ptr = GState->story + GState->header.objtab_addr;
    ptr += 31 * sizeof (uint16);  // skip properties defaults table
    ptr += 9 * (objid-1);  // find object in object table
    return ptr;
}
#endif

static void opcode_test_attr(void)
{
    const uint16 objid = GState->operands[0];
    const uint16 attrid = GState->operands[1];
    uint8 *ptr = getObjectPtr(objid);

    if (GState->header.version <= 3) {
        ptr += (attrid / 8);
        doBranch((*ptr & (0x80 >> (attrid & 7))) ? 1 : 0);
    } else {
        GState->die("write me");
    }
}

static void opcode_set_attr(void)
{
    const uint16 objid = GState->operands[0];
    const uint16 attrid = GState->operands[1];
    uint8 *ptr = getObjectPtr(objid);

    if (GState->header.version <= 3) {
        ptr += (attrid / 8);
        *ptr |= 0x80 >> (attrid & 7);
    } else {
        GState->die("write me");
    }
}

static void opcode_clear_attr(void)
{
    const uint16 objid = GState->operands[0];
    const uint16 attrid = GState->operands[1];
    uint8 *ptr = objid ? getObjectPtr(objid) : NULL;

    if (ptr == NULL) {
        return;  // Zork 1 will trigger this on "go X" where "x" isn't a direction, so ignore it.
    }

    if (GState->header.version <= 3) {
        ptr += (attrid / 8);
        *ptr &= ~(0x80 >> (attrid & 7));
    } else {
        GState->die("write me");
    }
}

static uint8 *getObjectPtrParent(const uint8 *objptr)
{
    if (GState->header.version <= 3) {
        const uint16 parent = objptr[4];
        return parent ? getObjectPtr(parent) : NULL;
    } else {
        GState->die("write me");
        return NULL;
    }
}

static void unparentObject(const uint16 _objid)
{
    const uint16 objid = remap_objectid(_objid);
    uint8 *objptr = getObjectPtr(objid);
    uint8 *parentptr = getObjectPtrParent(objptr);
    if (parentptr != NULL) {  // if NULL, no need to remove it.
        uint8 *ptr = parentptr + 6;  // 4 to skip attrs, 2 to skip to child.
        while (*ptr != objid) { // if not direct child, look through sibling list...
            ptr = getObjectPtr(*ptr) + 5;  // get sibling field.
        }
        *ptr = *(objptr + 5);  // obj sibling takes obj's place.
    }
}

static void opcode_insert_obj(void)
{
    const uint16 objid = remap_objectid(GState->operands[0]);
    const uint16 dstid = remap_objectid(GState->operands[1]);

    uint8 *objptr = getObjectPtr(objid);
    uint8 *dstptr = getObjectPtr(dstid);

    if (GState->header.version <= 3) {
        unparentObject(objid);  // take object out of its original tree first.

        // now reinsert in the right place.
        *(objptr + 4) = (uint8) dstid;  // parent field: new destination
        *(objptr + 5) = *(dstptr + 6);  // sibling field: new dest's old child.
        *(dstptr + 6) = (uint8) objid;  // dest's child field: object being moved.
    } else {
        GState->die("write me");  // fields are different in ver4+.
    }
}

static void opcode_remove_obj(void)
{
    const uint16 objid = GState->operands[0];
    uint8 *objptr = getObjectPtr(objid);

    if (GState->header.version > 3) {
        GState->die("write me");  // fields are different in ver4+.
    } else {
        unparentObject(objid);  // take object out of its original tree first.

        // now clear out object's relationships...
        *(objptr + 4) = 0;  // parent field: zero.
        *(objptr + 5) = 0;  // sibling field: zero.
    }
}

static uint8 *getObjectProperty(const uint16 objid, const uint32 propid, uint8 *_size);
#ifndef MULTIZORK
static uint8 *getObjectProperty(const uint16 objid, const uint32 propid, uint8 *_size)
{
    uint8 *ptr = getObjectPtr(objid);

    if (GState->header.version <= 3) {
        ptr += 7;  // skip to properties address field.
        const uint16 addr = READUI16(ptr);
        ptr = GState->story + addr;
        ptr += (*ptr * 2) + 1;  // skip object name to start of properties.
        while (1) {
            const uint8 info = *(ptr++);
            const uint16 num = (info & 0x1F);  // 5 bits for the prop id.
            const uint8 size = ((info >> 5) & 0x7) + 1; // 3 bits for prop size.
            // these go in descending numeric order, and should fail
            //  the interpreter if missing. We use 0xFFFFFFFF internally to mean "first property".
            if ((num == propid) || (propid == 0xFFFFFFFF)) {  // found it?
                if (_size) {
                    *_size = size;
                }
                return ptr;
            } else if (num < propid) {  // we're past it.
                break;
            }

            ptr += size;  // try the next property.
        }
    } else {
        GState->die("write me");
    }

    return NULL;
}
#endif

// this returns the zscii string for the object!
static const uint8 *getObjectShortName(const uint16 objid)
{
    const uint8 *ptr = getObjectPtr(objid);
    if (GState->header.version <= 3) {
        ptr += 7;  // skip to properties address field.
        const uint16 addr = READUI16(ptr);
        return GState->story + addr + 1;  // +1 to skip z-char count.
    } else {
        GState->die("write me");
    }

    return NULL;
}

static void opcode_put_prop(void)
{
    const uint16 objid = GState->operands[0];
    const uint16 propid = GState->operands[1];
    const uint16 value = GState->operands[2];
    uint8 size = 0;
    uint8 *ptr = getObjectProperty(objid, propid, &size);

    if (!ptr) {
        GState->die("Lookup on missing object property (obj=%X, prop=%X)", (unsigned int) objid, (unsigned int) propid);
    } else if ((size_t)(ptr - GState->story) + size > GState->header.staticmem_addr) {
        return;
    } else if (size == 1) {
        *ptr = (value & 0xFF);
    } else {
        WRITEUI16(ptr, value);
    }
}

static uint16 getDefaultObjectProperty(const uint16 propid)
{
    if ( ((GState->header.version <= 3) && (propid > 31)) ||
         ((GState->header.version >= 4) && (propid > 63)) ) {
        FIXME("Should we die here?");
        return 0;
    }

    const uint8 *values = (GState->story + GState->header.objtab_addr);
    values += (propid-1) * sizeof (uint16);
    const uint16 result = READUI16(values);
    return result;
}

static void opcode_get_prop(void)
{
    uint8 *store = varAddress(*(GState->pc++), 1, 0);
    const uint16 objid = GState->operands[0];
    const uint16 propid = GState->operands[1];
    uint16 result = 0;
    uint8 size = 0;
    uint8 *ptr = getObjectProperty(objid, propid, &size);

    if (!ptr) {
        result = getDefaultObjectProperty(propid);
    } else if (size == 1) {
        result = *ptr;
    } else {
        result = READUI16(ptr);
    }

    WRITEUI16(store, result);
}

static void opcode_get_prop_addr(void)
{
    uint8 *store = varAddress(*(GState->pc++), 1, 0);
    const uint16 objid = GState->operands[0];
    const uint16 propid = GState->operands[1];
    uint8 *ptr = getObjectProperty(objid, propid, NULL);
    const uint16 result = ptr ? ((uint16) (ptr-GState->story)) : 0;
    WRITEUI16(store, result);
}

static void opcode_get_prop_len(void)
{
    uint8 *store = varAddress(*(GState->pc++), 1, 0);
    uint16 result;

    if (GState->operands[0] == 0) {
        result = 0;  // this must return 0, to avoid a bug in older Infocom games.
    } else if (GState->header.version <= 3) {
        const uint16 offset = GState->operands[0];
        const uint8 *ptr = get_virtualized_mem_ptr(offset);
        const uint8 info = ptr[-1];  // the size field.
        result = ((info >> 5) & 0x7) + 1; // 3 bits for prop size.
    } else {
        GState->die("write me");
    }

    WRITEUI16(store, result);
}

static void opcode_get_next_prop(void)
{
    uint8 *store = varAddress(*(GState->pc++), 1, 0);
    const uint16 objid = GState->operands[0];
    const int firstProp = (GState->operands[1] == 0);
    uint16 result = 0;
    uint8 size = 0;
    uint8 *ptr = getObjectProperty(objid, firstProp ? 0xFFFFFFFF : GState->operands[1], &size);

    if (!ptr) {
        GState->die("get_next_prop on missing property obj=%X, prop=%X", (unsigned int) objid, (unsigned int) GState->operands[1]);
    } else if (GState->header.version <= 3) {
        result = ptr[firstProp ? -1 : ((sint8) size)] & 0x1F;  // 5 bits for the prop id.
    } else {
        GState->die("write me");
    }
    WRITEUI16(store, result);
}

static void opcode_jin(void)
{
    const uint16 objid = GState->operands[0];
    const uint16 parentid = GState->operands[1];
    const uint8 *objptr = objid ? getObjectPtr(objid) : NULL;

    if (objptr == NULL) {
        return;  // Zork 1 will trigger this on "go X" where "x" isn't a direction.
    }

    if (GState->header.version <= 3) {
        doBranch((((uint16) objptr[4]) == parentid) ? 1 : 0);
    } else {
        GState->die("write me");  // fields are different in ver4+.
    }
}

static uint16 getObjectRelationship(const uint16 objid, const uint8 relationship)
{
    const uint8 *objptr = getObjectPtr(objid);

    if (GState->header.version <= 3) {
        return objptr[relationship];
    } else {
        GState->die("write me");  // fields are different in ver4+.
    }
    return 0;
}

static void opcode_get_parent(void)
{
    uint8 *store = varAddress(*(GState->pc++), 1, 0);
    const uint16 result = getObjectRelationship(GState->operands[0], 4);
    WRITEUI16(store, result);
}

static void opcode_get_sibling(void)
{
    uint8 *store = varAddress(*(GState->pc++), 1, 0);
    const uint16 result = getObjectRelationship(GState->operands[0], 5);
    WRITEUI16(store, result);
    doBranch((result != 0) ? 1: 0);
}

static void opcode_get_child(void)
{
    uint8 *store = varAddress(*(GState->pc++), 1, 0);
    const uint16 result = getObjectRelationship(GState->operands[0], 6);
    WRITEUI16(store, result);
    doBranch((result != 0) ? 1: 0);
}

static void opcode_new_line(void)
{
    GState->writestr("\n", 1);
}

static char decode_zscii_char(const uint16 val)
{
    char ch = 0;

    FIXME("ver6+ has a few more valid codes");

    // only a few values are valid ZSCII codes for output.
    if ((val >= 32) && (val <= 126)) {
        ch = (char) val;  // FIXME: we assume you have an ASCII terminal for now.
    } else if (val == 13) {  // newline
        ch = '\n';
    } else if (val == 0) {
        /* val==0 is "valid" but produces no output. */ ;
    } else if ((val >= 155) && (val <= 251)) {
        { FIXME("write me: extended ZSCII characters"); ch = '?'; }
    } else {
        ch = '?';  // this is illegal, but we'll be nice.
    }
    return ch;
}

static uintptr decode_zscii(const uint8 *_str, const int abbr, char *buf, uintptr *_buflen)
{
    // ZCSII encoding is so nasty.
    uintptr buflen = *_buflen;
    uintptr decoded_chars = 0;
    const uint8 *str = _str;
    uint16 code = 0;
    uint8 alphabet = 0;
    uint8 useAbbrTable = 0;
    uint8 zscii_collector = 0;
    uint16 zscii_code = 0;

    do {
        if ((size_t)(str - GState->story) + 1 >= GState->story_len) {
            GState->die("Unterminated story text");
        }
        code = READUI16(str);

        // characters are 5 bits each, packed three to a 16-bit word.
        sint8 i;
        for (i = 10; i >= 0; i -= 5) {
            int newshift = 0;
            char printVal = 0;
            const uint8 ch = ((code >> i) & 0x1F);

            if (zscii_collector) {
                if (zscii_collector == 2) {
                    zscii_code |= ((uint16) ch) << 5;
                } else {
                    zscii_code |= ((uint16) ch);
                }

                zscii_collector--;
                if (!zscii_collector) {
                    printVal = decode_zscii_char(zscii_code);
                    if (printVal) {
                        decoded_chars++;
                        if (buflen) {
                            *(buf++) = printVal;
                            buflen--;
                        }
                    }
                    alphabet = useAbbrTable = 0;
                    zscii_code = 0;
                }
                continue;
            } else if (useAbbrTable) {
                if (abbr) {
                    GState->die("Abbreviation strings can't use abbreviations");
                }
                //FIXME("Make sure offset is sane");
                const uintptr index = ((32 * (((uintptr) useAbbrTable) - 1)) + (uintptr) ch);
                const uint8 *ptr = (GState->story + GState->header.abbrtab_addr) + (index * sizeof (uint16));
                const uint16 abbraddr = READUI16(ptr);
                uintptr abbr_decoded_chars = buflen;
                if (abbraddr * 2u + 1 >= GState->story_len) GState->die("Invalid text abbreviation");
                decode_zscii(GState->story + (abbraddr * sizeof (uint16)), 1, buf, &abbr_decoded_chars);
                decoded_chars += abbr_decoded_chars;
                buf += (buflen < abbr_decoded_chars) ? buflen : abbr_decoded_chars;
                buflen = (buflen < abbr_decoded_chars) ? 0 : (buflen - abbr_decoded_chars);
                useAbbrTable = 0;
                alphabet = 0;  // FIXME: no shift locking in ver3+, but ver1 needs it.
                continue;
            }

            switch (ch) {
                case 0:
                    printVal = ' ';
                    break;

                case 1:
                    if (GState->header.version == 1) {
                        printVal = '\n';
                    } else {
                        useAbbrTable = 1;
                    }
                    break;

                case 2:
                case 3:
                    if (GState->header.version <= 2) {
                        GState->die("write me: handle ver1/2 alphabet shifting");
                    } else {
                        useAbbrTable = ch;
                    }
                    break;

                case 4:
                case 5:
                    if (GState->header.version <= 2) {
                        GState->die("write me: handle ver1/2 alphabet shift locking");
                    } else {
                        newshift = 1;
                        alphabet = ch - 3;
                    }
                    break;

                default:
                    if ((ch == 6) && (alphabet == 2)) {
                        zscii_collector = 2;
                    } else {
                        printVal = GState->alphabet_table[(alphabet*26) + (ch-6)];
                    }
                    break;
            }

            if (printVal) {
                decoded_chars++;
                if (buflen) {
                    *(buf++) = printVal;
                    buflen--;
                }
            }

            if (alphabet && !newshift) {
                alphabet = 0;
            }
        }

        // there is no NULL terminator, you look for a word with the top bit set.
    } while ((code & (1<<15)) == 0);

    *_buflen = decoded_chars;
    return str - _str;
}

static uintptr print_zscii(const uint8 *_str, const int abbr)
{
    static char buffer[4096];
    uintptr length = sizeof(buffer);
    uintptr consumed = decode_zscii(_str, abbr, buffer, &length);
    if (length > sizeof(buffer)) GState->die("Story text exceeds output buffer");
    GState->writestr(buffer, length);
    return consumed;
}

static void opcode_print(void)
{
    GState->pc += print_zscii(GState->pc, 0);
}

static void opcode_print_num(void)
{
    port_print_number((sint16) GState->operands[0]);
}

static void opcode_print_char(void)
{
    const char ch = decode_zscii_char(GState->operands[0]);
    if (ch) {
        GState->writestr(&ch, 1);
    }
}

static void opcode_print_ret(void)
{
    GState->pc += print_zscii(GState->pc, 0);
    GState->writestr("\n", 1);
    doReturn(1);
}

static void opcode_print_obj(void)
{
    if (GState->operands[0] == 0 && port_print_missing_object(GState->logical_pc)) return;
    const uint8 *ptr = getObjectPtr(GState->operands[0]);
    if (GState->header.version <= 3) {
        ptr += 7;  // skip to properties field.
        const uint16 addr = READUI16(ptr);  // dereference to get to property table.
        print_zscii(GState->story + addr + 1, 0);
    } else {
        GState->die("write me");
    }
}

static void opcode_print_addr(void)
{
    print_zscii(GState->story + GState->operands[0], 0);
}

static void opcode_print_paddr(void)
{
    // Some states in the original Zork II reference a description beyond EOF.
    // Continue the surrounding sentence without reading outside the cartridge story.
    if (GState->operands[0] * 2u + 1 >= GState->story_len) {
        GState->writestr("something", 9);
    } else {
        print_zscii(unpackAddress(GState->operands[0]), 0);
    }
}

static uint32 random_seed;
static int randomNumber(void)
{
    // this is POSIX.1-2001's potentially bad suggestion, but we're not exactly doing cryptography here.
    random_seed = random_seed * 1103515245u + 12345u;
    return (int) ((random_seed >> 16) & 32767u);
}

static uint16 doRandom(const sint16 range)
{
    uint16 result = 0;
    if (range == 0) {  // reseed in "most random way"
        random_seed = port_entropy();
    } else if (range < 0) {  // reseed with specific value
        random_seed = -range;
    } else {
        const uint16 lo = 1;
        const uint16 hi = (uint16) range;
        result = port_unsigned_remainder((uint16) randomNumber(), (hi + 1) - lo) + lo;
        if (!result)
            result = 1;
    }
    return result;
}

static void opcode_random(void)
{
    uint8 *store = varAddress(*(GState->pc++), 1, 0);
    const sint16 range = (sint16) GState->operands[0];
    const uint16 result = doRandom(range);
    WRITEUI16(store, result);
}

/* Version 3 keeps six Z-characters per dictionary key. */
static uint16 dictionary_word(const uint8 *word, unsigned length,
                              const uint8 *dictionary, unsigned stride, unsigned entries)
{
    uint8 zchars[9] = {5, 5, 5, 5, 5, 5};
    unsigned used = 0;
    for (unsigned i = 0; i < length && used < 6; i++) {
        uint8 ch = word[i];
        if (ch >= 'a' && ch <= 'z') {
            zchars[used++] = ch - 'a' + 6;
        } else {
            zchars[used++] = 5;
            unsigned c = 1;
            while (c < 26 && GState->alphabet_table[52 + c] != ch) c++;
            if (c < 26) {
                zchars[used++] = c + 6;
            } else {
                zchars[used++] = 6;
                zchars[used++] = ch >> 5;
                zchars[used++] = ch & 31;
            }
        }
    }
    uint16 first = (zchars[0] << 10) | (zchars[1] << 5) | zchars[2];
    uint16 second = 0x8000 | (zchars[3] << 10) | (zchars[4] << 5) | zchars[5];
    for (unsigned i = 0; i < entries; i++, dictionary += stride) {
        if (read_word(dictionary) == first && read_word(dictionary + 2) == second)
            return (uint16)(dictionary - GState->story);
    }
    return 0;
}

static bool is_dictionary_separator(uint8 ch, const uint8 *separators, unsigned count)
{
    for (unsigned i = 0; i < count; i++) if (separators[i] == ch) return true;
    return false;
}

static void tokenizeUserInput(void)
{
    const uint8 *input = GState->story + GState->operands[0] + 1;
    uint8 *parse = GState->story + GState->operands[1];
    unsigned capacity = parse[0], words = 0;
    const uint8 *separators = GState->story + GState->header.dict_addr;
    unsigned count = *separators++;
    const uint8 *dictionary = separators + count;
    unsigned stride = *dictionary++;
    int entries = (sint16)read_word(dictionary);
    if (entries < 0) entries = -entries;
    dictionary += 2;

    unsigned offset = 0;
    while (input[offset] && words < capacity) {
        if (input[offset] == ' ') {
            offset++;
            continue;
        }
        unsigned start = offset++;
        if (!is_dictionary_separator(input[start], separators, count)) {
            while (input[offset] && input[offset] != ' ' &&
                   !is_dictionary_separator(input[offset], separators, count)) offset++;
        }
        uint8 *token = parse + 2 + 4 * words++;
        write_word(token, dictionary_word(input + start, offset - start, dictionary, stride, entries));
        token[2] = offset - start;
        token[3] = start + 1;
    }
    parse[1] = words;
}

static void opcode_read(void)
{
    GState->waiting_for_line = 1;
    GState->step_completed = 1;
}

static void opcode_output_stream(void)
{
    if ((sint16) GState->operands[0] == 1) port_screen_enabled(true);
    else if ((sint16) GState->operands[0] == -1) port_screen_enabled(false);
    else if ((sint16) GState->operands[0] == 3) port_open_stream(GState->operands[1]);
    else if ((sint16) GState->operands[0] == -3) port_close_stream();
}

static void opcode_verify(void)
{
    doBranch((GState->calculated_checksum == GState->header.story_checksum) ? 1 : 0);
}

static void opcode_split_window(void)
{
    // it's illegal for a game to use this opcode if the implementation reported it doesn't support it.
    // other targets (libretro, etc) do support it, and set this flag, but the
    // core MojoZork, which is stdio-only, does not.
    if ((GState->header.flags1 & (1<<5)) == 0) {
        GState->die("split_window called but implementation doesn't support it!");
    }

    const uint16 oldval = GState->upper_window_line_count;
    GState->upper_window_line_count = GState->operands[0];
    if (GState->split_window) {
        GState->split_window(oldval, GState->upper_window_line_count);
    }
}

static void opcode_set_window(void)
{
    // it's illegal for a game to use this opcode if the implementation reported it doesn't support it.
    // other targets (libretro, etc) do support it, and set this flag, but the
    // core MojoZork, which is stdio-only, does not.
    if ((GState->header.flags1 & (1<<5)) == 0) {
        GState->die("set_window called but implementation doesn't support it!");
    }
    const uint16 oldval = GState->current_window;
    GState->current_window = GState->operands[0];
    if (GState->set_window) {
        GState->set_window(oldval, GState->current_window);
    }
}

static void opcode_restart(void)
{
    port_restart();
}

static void opcode_save(void)
{
    doBranch(port_save() ? 1 : 0);
}

static void opcode_restore(void)
{
    if (!port_restore()) doBranch(0);
}

static void opcode_quit(void)
{
    GState->quit = 1;
    GState->step_completed = 1;  // possibly time to break out of the Z-Machine simulation loop.
}

static void opcode_nop(void)
{
    // that's all, folks.
}

static int parseOperand(const uint8 optype, uint16 *operand)
{
    switch (optype) {
        case 0: *operand = (uint16) READUI16(GState->pc); return 1;  // large constant (uint16)
        case 1: *operand = *(GState->pc++); return 1;  // small constant (uint8)
        case 2: { // variable
            const uint8 *addr = varAddress(*(GState->pc++), 0, 0);
            *operand = READUI16(addr);
            return 1;
        }
        case 3: break;  // omitted altogether, we're done.
    }

    return 0;
}

static uint8 parseVarOperands(uint16 *operands)
{
    const uint8 operandTypes = *(GState->pc++);
    uint8 shifter = 6;
    uint8 i;

    for (i = 0; i < 4; i++) {
        const uint8 optype = (operandTypes >> shifter) & 0x3;
        shifter -= 2;
        if (!parseOperand(optype, operands + i)) {
            break;
        }
    }

    return i;
}

static void runInstruction(void)
{
    FIXME("verify PC is sane");

    GState->logical_pc = (uint32) (GState->pc - GState->story);
    uint8 opcode = *(GState->pc++);

    const Opcode *op = NULL;

    const int extended = ((opcode == 190) && (GState->header.version >= 5)) ? 1 : 0;
    if (extended) {
        opcode = *(GState->pc++);
        if (opcode >= (sizeof (GState->extended_opcodes) / sizeof (GState->extended_opcodes[0]))) {
            GState->die("Unsupported or unknown extended opcode #%u", (unsigned int) opcode);
        }
        GState->operand_count = parseVarOperands(GState->operands);
        op = &GState->extended_opcodes[opcode];
    } else {
        if (opcode <= 127) {   // 2OP
            GState->operand_count = 2;
            parseOperand(((opcode >> 6) & 0x1) ? 2 : 1, GState->operands + 0);
            parseOperand(((opcode >> 5) & 0x1) ? 2 : 1, GState->operands + 1);
        } else if (opcode <= 175) {  // 1OP
            GState->operand_count = 1;
            const uint8 optype = (opcode >> 4) & 0x3;
            parseOperand(optype, GState->operands);  // 1OP or 0OP
        } else if (opcode <= 191) {  // 0OP
            GState->operand_count = 0;
        } else if (opcode > 191) {  // VAR
            const int takes8 = ((opcode == 236) || (opcode == 250));  // call_vs2 and call_vn2 take up to EIGHT arguments!
            if (!takes8) {
                GState->operand_count = parseVarOperands(GState->operands);
            } else {
                GState->operand_count = parseVarOperands(GState->operands);
                if (GState->operand_count == 4) {
                    GState->operand_count += parseVarOperands(GState->operands + 4);
                } else {
                    GState->pc++;  // skip the next byte, since we don't have any more args.
                }
            }
        }

        op = &GState->opcodes[opcode];
    }

    if (!op->name) {
        GState->die("Unsupported or unknown %sopcode #%u", extended ? "extended " : "", (unsigned int) opcode);
    } else if (!op->fn) {
        GState->die("Unimplemented %sopcode #%d ('%s')", extended ? "extended " : "", (unsigned int) opcode, op->name);
    } else {
        #if MOJOZORK_DEBUGGING
        dbg("pc=%X %sopcode=%u ('%s') [", (unsigned int) GState->logical_pc, extended ? "ext " : "", opcode, op->name);
        if (GState->operand_count)
        {
            uint8 i;
            for (i = 0; i < GState->operand_count-1; i++)
                dbg("%X,", (unsigned int) GState->operands[i]);
            dbg("%X", (unsigned int) GState->operands[i]);
        }
        dbg("]\n");
        #endif

        op->fn();
        GState->instructions_run++;
    }
}

static void initAlphabetTable(void)
{
    FIXME("ver5+ specifies alternate tables in the header");

    char *ptr = GState->alphabet_table;
    uint8 i;

    // alphabet A0
    for (i = 0; i < 26; i++) {
        *(ptr++) = 'a' + i;
    }

    // alphabet A1
    for (i = 0; i < 26; i++) {
        *(ptr++) = 'A' + i;
    }

    // alphabet A2
    *(ptr++) = '\0';

    if (GState->header.version != 1) {
        *(ptr++) = '\n';
    }

    for (i = 0; i < 10; i++) {
        *(ptr++) = '0' + i;
    }
    *(ptr++) = '.';
    *(ptr++) = ',';
    *(ptr++) = '!';
    *(ptr++) = '?';
    *(ptr++) = '_';
    *(ptr++) = '#';
    *(ptr++) = '\'';
    *(ptr++) = '"';
    *(ptr++) = '/';
    *(ptr++) = '\\';

    if (GState->header.version == 1) {
        *(ptr++) = '<';
    }

    *(ptr++) = '-';
    *(ptr++) = ':';
    *(ptr++) = '(';
    *(ptr++) = ')';
}

static void inititialOpcodeTableSetup(void)
{
    FIXME("lots of missing instructions here.  :)");

    memset(GState->opcodes, '\0', sizeof (GState->opcodes));
    memset(GState->extended_opcodes, '\0', sizeof (GState->extended_opcodes));

    Opcode *opcodes = GState->opcodes;

    #define OPCODE(num, opname) opcodes[num].name = #opname; opcodes[num].fn = opcode_##opname
    #define OPCODE_WRITEME(num, opname) opcodes[num].name = #opname

    // this is the basic ver1 opcode table, then we can patch it after.
    // most early Infocom games are version 3, but apparently ver1 is in the wild...

    // 2-operand instructions...
    OPCODE(1, je);
    OPCODE(2, jl);
    OPCODE(3, jg);
    OPCODE(4, dec_chk);
    OPCODE(5, inc_chk);
    OPCODE(6, jin);
    OPCODE(7, test);
    OPCODE(8, or);
    OPCODE(9, and);
    OPCODE(10, test_attr);
    OPCODE(11, set_attr);
    OPCODE(12, clear_attr);
    OPCODE(13, store);
    OPCODE(14, insert_obj);
    OPCODE(15, loadw);
    OPCODE(16, loadb);
    OPCODE(17, get_prop);
    OPCODE(18, get_prop_addr);
    OPCODE(19, get_next_prop);
    OPCODE(20, add);
    OPCODE(21, sub);
    OPCODE(22, mul);
    OPCODE(23, div);
    OPCODE(24, mod);

    // 1-operand instructions...
    OPCODE(128, jz);
    OPCODE(129, get_sibling);
    OPCODE(130, get_child);
    OPCODE(131, get_parent);
    OPCODE(132, get_prop_len);
    OPCODE(133, inc);
    OPCODE(134, dec);
    OPCODE(135, print_addr);
    OPCODE(137, remove_obj);
    OPCODE(138, print_obj);
    OPCODE(139, ret);
    OPCODE(140, jump);
    OPCODE(141, print_paddr);
    OPCODE(142, load);
    OPCODE(143, not);

    // 0-operand instructions...
    OPCODE(176, rtrue);
    OPCODE(177, rfalse);
    OPCODE(178, print);
    OPCODE(179, print_ret);
    OPCODE(180, nop);
    OPCODE(181, save);
    OPCODE(182, restore);
    OPCODE(183, restart);
    OPCODE(184, ret_popped);
    OPCODE(185, pop);
    OPCODE(186, quit);
    OPCODE(187, new_line);

    // variable operand instructions...
    OPCODE(224, call);
    OPCODE(225, storew);
    OPCODE(226, storeb);
    OPCODE(227, put_prop);
    OPCODE(228, read);
    OPCODE(229, print_char);
    OPCODE(230, print_num);
    OPCODE(231, random);
    OPCODE(232, push);
    OPCODE(233, pull);

    if (GState->header.version < 3) {  // most Infocom games are version 3.
        return;  // we're done.
    }

    OPCODE(188, show_status);
    OPCODE(189, verify);
    OPCODE(234, split_window);
    OPCODE(235, set_window);
    OPCODE(243, output_stream);
    opcodes[244].fn = opcode_nop;
    opcodes[245].fn = opcode_nop;

    if (GState->header.version < 4) {
        return;  // we're done.
    }

    // show_status is illegal in ver4+, but a build of Wishbringer
    //  accidentally calls it, so always treat it as NOP instead.
    opcodes[188].fn = opcode_nop;

    OPCODE_WRITEME(25, call_2s);
    OPCODE_WRITEME(180, save_ver4);
    OPCODE_WRITEME(224, call_vs);
    OPCODE_WRITEME(228, sread_ver4);
    OPCODE_WRITEME(236, call_vs2);
    OPCODE_WRITEME(237, erase_window);
    OPCODE_WRITEME(238, erase_line);
    OPCODE_WRITEME(239, set_cursor);
    OPCODE_WRITEME(240, get_cursor);
    OPCODE_WRITEME(241, set_text_style);
    OPCODE_WRITEME(242, buffer_mode);
    OPCODE_WRITEME(246, read_char);
    OPCODE_WRITEME(247, scan_table);

    if (GState->header.version < 5) {
        return;  // we're done.
    }

    OPCODE_WRITEME(26, call_2n);
    OPCODE_WRITEME(27, set_colour);
    OPCODE_WRITEME(28, throw);
    OPCODE_WRITEME(136, call_1s);
    OPCODE_WRITEME(143, call_1n);
    OPCODE_WRITEME(185, catch);
    OPCODE_WRITEME(191, piracy);
    OPCODE_WRITEME(228, aread);
    OPCODE_WRITEME(243, output_stream_ver5);
    OPCODE_WRITEME(245, sound_effect_ver5);
    OPCODE_WRITEME(248, not_ver5);
    OPCODE_WRITEME(249, call_vn);
    OPCODE_WRITEME(250, call_vn2);
    OPCODE_WRITEME(251, tokenise);
    OPCODE_WRITEME(252, encode_text);
    OPCODE_WRITEME(253, copy_table);
    OPCODE_WRITEME(254, print_table);
    OPCODE_WRITEME(255, check_arg_count);

    // this is the "save" and "restore" opcodes in ver1-4; illegal in ver5+.
    //  in ver5+, they use extended opcode 0 and 1 for these.
    opcodes[180].name = opcodes[181].name = NULL;
    opcodes[180].fn = opcodes[181].fn = NULL;

    // We special-case this later, so no function pointer supplied.
    opcodes[190].name = "extended";

    // extended opcodes in ver5+ ...
    opcodes = GState->extended_opcodes;
    OPCODE_WRITEME(0, save_ext);
    OPCODE_WRITEME(1, restore_ext);
    OPCODE_WRITEME(2, log_shift);
    OPCODE_WRITEME(3, art_shift);
    OPCODE_WRITEME(4, set_font);
    OPCODE_WRITEME(9, save_undo);
    OPCODE_WRITEME(10, restore_undo);
    OPCODE_WRITEME(11, print_unicode);
    OPCODE_WRITEME(12, check_unicode);
    OPCODE_WRITEME(13, set_true_colour);
    opcodes = GState->opcodes;

    if (GState->header.version < 6) {
        return;  // we're done.
    }

    OPCODE_WRITEME(27, set_colour_ver6);
    OPCODE_WRITEME(27, throw_ver6);
    OPCODE_WRITEME(185, catch_ver6);
    OPCODE_WRITEME(233, pull_ver6);
    OPCODE_WRITEME(238, erase_line_ver6);
    OPCODE_WRITEME(239, set_cursor_ver6);
    OPCODE_WRITEME(243, output_stream_ver6);
    OPCODE_WRITEME(248, not_ver6);

    opcodes = GState->extended_opcodes;
    OPCODE_WRITEME(4, set_font_ver6);
    OPCODE_WRITEME(5, draw_picture);
    OPCODE_WRITEME(6, picture_data);
    OPCODE_WRITEME(7, erase_picture);
    OPCODE_WRITEME(8, set_margins);
    OPCODE_WRITEME(13, set_true_colour_ver6);
    // 14 and 15 are unused.
    OPCODE_WRITEME(16, move_window);
    OPCODE_WRITEME(17, window_size);
    OPCODE_WRITEME(18, window_style);
    OPCODE_WRITEME(19, get_wind_prop);
    OPCODE_WRITEME(20, scroll_window);
    OPCODE_WRITEME(21, pop_stack);
    OPCODE_WRITEME(22, read_mouse);
    OPCODE_WRITEME(23, mouse_window);
    OPCODE_WRITEME(24, push_stack);
    OPCODE_WRITEME(25, put_wind_prop);
    OPCODE_WRITEME(26, print_form);
    OPCODE_WRITEME(27, make_menu);
    OPCODE_WRITEME(28, picture_table);
    OPCODE_WRITEME(29, buffer_screen);

    #undef OPCODE
    #undef OPCODE_WRITEME
}

static void initOpcodeTable(void)
{
    inititialOpcodeTableSetup();

    // finalize the opcode table...
    Opcode *opcodes = GState->opcodes;
    for (uint8 i = 32; i <= 127; i++) {   // 2OP opcodes repeating with different operand forms.
        opcodes[i] = opcodes[i % 32];
    }
    for (uint8 i = 144; i <= 175; i++) {  // 1OP opcodes repeating with different operand forms.
        opcodes[i] = opcodes[128 + (i % 16)];
    }
    for (uint8 i = 192; i <= 223; i++) {  // 2OP opcodes repeating with VAR operand forms.
        opcodes[i] = opcodes[i % 32];
    }
}

// call this before we make any changes to GState->story.
static void calculateActualChecksum(void)
{
    uint32 total = GState->header.story_len;
    uint16 checksum = 0;

    // 11.1.6: The file length stored at $1a is actually divided by a constant, depending on the Version, to make it fit into a header word. This constant is 2 for Versions 1 to 3, 4 for Versions 4 to 5 or 8 for Versions 6 and later.
    if (GState->header.version <= 3) {
        total *= 2;
    } else if (GState->header.version <= 5) {
        total *= 4;
    } else {
        total *= 8;
    }

    const uint8 *ptr = GState->story;
    for (uint32 i = 0x40; i < total; i++) {
        checksum += ptr[i];
    }

    GState->calculated_checksum = checksum;
}

static void initStory(uint8 *story, const uint32 storylen)
{
    GState->story = story;
    GState->story_len = (uintptr) storylen;
    GState->instructions_run = 0;
    GState->pc = 0;
    GState->logical_pc = 0;
    GState->quit = 0;
    GState->waiting_for_line = 0;
    GState->step_completed = 0;
    GState->current_window = 0;
    GState->upper_window_line_count = 0;
    memset(GState->stack, '\0', sizeof (GState->stack));
    memset(GState->operands, '\0', sizeof (GState->operands));
    GState->operand_count = 0;
    GState->sp = NULL;  // stack pointer
    GState->bp = 0;  // base pointer

    memset(&GState->header, '\0', sizeof (GState->header));

    //GState->story[1] &= ~(1<<3);  // this is the infamous "Tandy Bit". Turn it off.
    GState->story[1] &= ~(1<<4);  // status is drawn by the GBA UI.

    // since the base interpreter doesn't show a status bar, as it's just
    //  stdout, other targets that _can_ will want to provide a buffer for the
    //  status bar, 1 char larger than they need (for a null-terminator),
    //  and also do `GState->story[1] &= ~(1<<4);` so the game knows that a
    //  status bar is available. opcode_read and opcode_show_status in this
    //  implementation will fill in the buffer if available, and the target
    //  can display it as appropriate.
    //GState->story[1] &= ~(1<<4);  // uncommenting this tells the game that a status bar can be displayed.

    // since the base interpreter doesn't support window-splitting, as it's
    //  just stdout, other targets that _can_ will want to manage this in
    //  their writestr implementation, checking GState->current_window and
    //  GState->upper_window_line_count and do `GState->story[1] |= (1<<5);`
    //  so the game knows that window-splitting is available, and do the right
    //  thing when GState->writestr is called. The base interpreter also
    //  provides optional hooks so the target can know that split_window or
    //  set_window has been called, so it can manage state.
    //GState->story[1] |= (1<<5);  // uncommenting this tells the game that a window-splitting is supported.

    const uint8 *ptr = GState->story;
    GState->header.version = READUI8(ptr);
    GState->header.flags1 = READUI8(ptr);
    GState->header.release = READUI16(ptr);
    GState->header.himem_addr = READUI16(ptr);
    GState->header.pc_start = READUI16(ptr);
    GState->header.dict_addr = READUI16(ptr);
    GState->header.objtab_addr = READUI16(ptr);
    GState->header.globals_addr = READUI16(ptr);
    GState->header.staticmem_addr = READUI16(ptr);
    GState->header.flags2 = READUI16(ptr);
    GState->header.serial_code[0] = READUI8(ptr);
    GState->header.serial_code[1] = READUI8(ptr);
    GState->header.serial_code[2] = READUI8(ptr);
    GState->header.serial_code[3] = READUI8(ptr);
    GState->header.serial_code[4] = READUI8(ptr);
    GState->header.serial_code[5] = READUI8(ptr);
    GState->header.abbrtab_addr = READUI16(ptr);
    GState->header.story_len = READUI16(ptr);
    GState->header.story_checksum = READUI16(ptr);


    dbg(" - version %u\n", (unsigned int) GState->header.version);
    dbg(" - flags 0x%X\n", (unsigned int) GState->header.flags1);
    dbg(" - release %u\n", (unsigned int) GState->header.release);
    dbg(" - high memory addr %X\n", (unsigned int) GState->header.himem_addr);
    dbg(" - program counter start %X\n", (unsigned int) GState->header.pc_start);
    dbg(" - dictionary address %X\n", (unsigned int) GState->header.dict_addr);
    dbg(" - object table address %X\n", (unsigned int) GState->header.objtab_addr);
    dbg(" - globals address %X\n", (unsigned int) GState->header.globals_addr);
    dbg(" - static memory address %X\n", (unsigned int) GState->header.staticmem_addr);
    dbg(" - flags2 0x%X\n", (unsigned int) GState->header.flags2);
    dbg(" - serial '%s'\n", GState->header.serial_code);
    dbg(" - abbreviations table address %X\n", (unsigned int) GState->header.abbrtab_addr);
    dbg(" - story length %u\n", (unsigned int) GState->header.story_len);
    dbg(" - story checksum 0x%X\n", (unsigned int) GState->header.story_checksum);

    if (GState->header.version != 3) {
        GState->die("FIXME: only version 3 is supported right now, this is %d", (int) GState->header.version);
    }

    calculateActualChecksum();
    initAlphabetTable();
    initOpcodeTable();

    FIXME("in ver6+, this is the address of a main() routine, not a raw instruction address.");
    GState->pc = GState->story + GState->header.pc_start;
    GState->logical_pc = (uint32) GState->header.pc_start;
    GState->bp = 0;
    GState->sp = GState->stack;
}
