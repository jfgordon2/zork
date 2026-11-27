#ifndef ZORK_VISUAL_STATE_H
#define ZORK_VISUAL_STATE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum { QUARTERS_UNKNOWN, QUARTERS_CAVE, QUARTERS_LOUIS, QUARTERS_BUNKS,
       QUARTERS_HAMMOCK, QUARTERS_AUSTERE, QUARTERS_ARABIAN };
enum { VISTA_UNKNOWN, VISTA_I, VISTA_II, VISTA_III, VISTA_IV };

void visual_state_reset(void);
void visual_state_observe(unsigned chapter, unsigned room, const char *text, size_t size);
uint8_t visual_state_save(void);
void visual_state_restore(uint8_t saved);
uint8_t visual_state_quarters(void);
uint8_t visual_state_vista(void);
uint8_t visual_state_parapet_dial(void);
uint8_t visual_state_mirror_arrow(void);
bool visual_state_mirror_pole_raised(void);
bool visual_state_mirror_pine_open(void);
bool visual_state_mist_view_dark(void);
bool visual_state_troll_disappeared(void);
bool visual_state_cyclops_clue_read(void);
void visual_state_mark_cyclops_clue_read(void);
uint16_t visual_state_wizard_spells(void);
void visual_state_restore_wizard_spells(uint16_t spells);
void visual_state_restore_z3(uint8_t parapet_dial, uint8_t mirror_arrow,
                             bool mirror_pole_raised, bool mirror_pine_open);

#endif
