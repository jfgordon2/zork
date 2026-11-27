#include "visual_state.h"
#include <stdbool.h>

static uint8_t quarters;
static uint8_t vista;
static uint8_t parapet_dial;
static uint8_t mirror_arrow;
static bool mirror_pole_raised, mirror_pine_open;
static bool mist_view_dark;
static bool troll_disappeared;
static bool cyclops_clue_read;
static uint16_t wizard_spells;
static char line[192];
static unsigned used;

static int contains(const char *text, const char *part) {
    for (; *text; text++) {
        unsigned i = 0;
        while (part[i] && text[i] == part[i]) i++;
        if (!part[i]) return 1;
    }
    return 0;
}

void visual_state_reset(void) {
    quarters = QUARTERS_UNKNOWN;
    vista = VISTA_UNKNOWN;
    parapet_dial = 1;
    mirror_arrow = 6; // The introductory description points west.
    mirror_pole_raised = false;
    mirror_pine_open = false;
    mist_view_dark = false;
    troll_disappeared = false;
    cyclops_clue_read = false;
    wizard_spells = 0;
    used = 0;
    line[0] = 0;
}

static uint8_t roman_after(const char *text, const char *marker) {
    for (; *text; text++) {
        unsigned i = 0;
        while (marker[i] && text[i] == marker[i]) i++;
        if (marker[i]) continue;
        const char *value = text + i;
        if (value[0] != 'I') return VISTA_UNKNOWN;
        if (value[1] == 'V') return VISTA_IV;
        if (value[1] != 'I') return VISTA_I;
        if (value[2] != 'I') return VISTA_II;
        return VISTA_III;
    }
    return VISTA_UNKNOWN;
}

static uint8_t direction_after(const char *text, const char *marker) {
    static const char *const directions[] = {
        "north", "northeast", "east", "southeast", "south", "southwest", "west", "northwest"
    };
    for (; *text; text++) {
        unsigned i = 0;
        while (marker[i] && text[i] == marker[i]) i++;
        if (marker[i]) continue;
        const char *value = text + i;
        for (unsigned d = 0; d < sizeof(directions) / sizeof(directions[0]); d++) {
            unsigned j = 0;
            while (directions[d][j] && value[j] == directions[d][j]) j++;
            if (!directions[d][j] && (!value[j] || value[j] == ' ' || value[j] == '.' || value[j] == '\n'))
                return d;
        }
        return mirror_arrow;
    }
    return mirror_arrow;
}

void visual_state_observe(unsigned chapter, unsigned room, const char *text, size_t size) {
    bool quarters_line = chapter == 1 && room == 69;
    bool mist_line = chapter == 1 && room == 224;
    bool troll_line = chapter == 0 && room == 127;
    bool vista_line = chapter == 2 && room == 124;
    bool mirror_line = chapter == 2 && room == 160;
    bool parapet_line = chapter == 2 && room == 36;
    if (!quarters_line && !mist_line && !troll_line && !vista_line && !mirror_line && !parapet_line) {
        used = 0;
        line[0] = 0;
        return;
    }
    // Story output can arrive one character at a time. Match its actual description,
    // rather than consuming another random number or guessing from the next RNG state.
    for (size_t i = 0; i < size; i++) {
        if (text[i] == '\n' || text[i] == '\r') { used = 0; line[0] = 0; continue; }
        if (used + 1 == sizeof(line)) {
            unsigned keep = used / 2;
            for (unsigned j = 0; j < keep; j++) line[j] = line[j + used - keep];
            used = keep;
        }
        line[used++] = text[i];
        line[used] = 0;
        if (quarters_line) {
            if (contains(line, "furs and old bones")) quarters = QUARTERS_CAVE;
            else if (contains(line, "Louis XIV")) quarters = QUARTERS_LOUIS;
            else if (contains(line, "bunk beds")) quarters = QUARTERS_BUNKS;
            else if (contains(line, "hammock")) quarters = QUARTERS_HAMMOCK;
            else if (contains(line, "monkish")) quarters = QUARTERS_AUSTERE;
            else if (contains(line, "seraglio")) quarters = QUARTERS_ARABIAN;
        } else if (mist_line) {
            if (contains(line, "You see only darkness.")) mist_view_dark = true;
            else if (contains(line, "You see a small room with a sign")) mist_view_dark = false;
        } else if (troll_line) {
            if (contains(line, "carcass has disappeared.")) troll_disappeared = true;
        } else if (vista_line) {
            uint8_t state = roman_after(line, "reads \"");
            if (state == VISTA_UNKNOWN) state = roman_after(line, "changes to \"");
            if (state != VISTA_UNKNOWN) vista = state;
        } else if (parapet_line) {
            const char *markers[] = {"dial now points to ", "indicator points to the number "};
            for (unsigned m = 0; m < sizeof(markers) / sizeof(markers[0]); m++) {
                for (const char *p = line; *p; p++) {
                    unsigned i = 0;
                    while (markers[m][i] && p[i] == markers[m][i]) i++;
                    if (!markers[m][i] && p[i] >= '1' && p[i] <= '8') parapet_dial = p[i] - '0';
                }
            }
        } else {
            mirror_arrow = direction_after(line, "arrow on the compass rose now indicates ");
            mirror_arrow = direction_after(line, "arrow and T-bar are pointing ");
            if (contains(line, "pole is now slightly above the floor")) mirror_pole_raised = true;
            if (contains(line, "pole is lowered into the channel")) mirror_pole_raised = false;
            if (contains(line, "pine wall swings open")) mirror_pine_open = true;
            if (contains(line, "as you leave, the door swings shut")) mirror_pine_open = false;
        }
    }
}

uint8_t visual_state_save(void) {
    return (quarters & 7) | ((vista & 7) << 3) | (mist_view_dark ? 0x40 : 0) |
           (troll_disappeared ? 0x80 : 0);
}

void visual_state_restore(uint8_t saved) {
    visual_state_reset();
    uint8_t saved_quarters = saved & 7;
    uint8_t saved_vista = (saved >> 3) & 7;
    if (saved_quarters <= QUARTERS_ARABIAN) quarters = saved_quarters;
    if (saved_vista <= VISTA_IV) vista = saved_vista;
    mist_view_dark = (saved & 0x40) != 0;
    troll_disappeared = (saved & 0x80) != 0;
}

uint8_t visual_state_quarters(void) { return quarters; }
uint8_t visual_state_vista(void) { return vista; }
uint8_t visual_state_parapet_dial(void) { return parapet_dial; }
uint8_t visual_state_mirror_arrow(void) { return mirror_arrow; }
bool visual_state_mirror_pole_raised(void) { return mirror_pole_raised; }
bool visual_state_mirror_pine_open(void) { return mirror_pine_open; }
bool visual_state_mist_view_dark(void) { return mist_view_dark; }
bool visual_state_troll_disappeared(void) { return troll_disappeared; }
bool visual_state_cyclops_clue_read(void) { return cyclops_clue_read; }
void visual_state_mark_cyclops_clue_read(void) { cyclops_clue_read = true; }
uint16_t visual_state_wizard_spells(void) { return wizard_spells; }
void visual_state_restore_wizard_spells(uint16_t spells) { wizard_spells = spells; }

void visual_state_restore_z3(uint8_t dial, uint8_t arrow, bool pole_raised, bool pine_open) {
    if (dial >= 1 && dial <= 8) parapet_dial = dial;
    if (arrow < 8) mirror_arrow = arrow;
    mirror_pole_raised = pole_raised;
    mirror_pine_open = pine_open;
}
