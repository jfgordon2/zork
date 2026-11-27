#ifndef ZORK_INTERACTION_H
#define ZORK_INTERACTION_H

#include "chapter.h"

enum { MAX_ACTIONS = 18, COMMAND_SIZE = 128, WORDS_PER_PHRASE = 3 };
enum { STORY_MENU_PROBE_BUDGET = 5000, STORY_MENU_PROBE_CHUNK = 200 };

typedef enum {
    DO_OBJECT, DO_LITERAL, PICK_TARGET, PICK_SELF, PICK_ITEM, PICK_DIRECTION, PICK_DIGIT, PICK_YEAR,
    SHOW_CATEGORIES, SHOW_ORDERS
} ActionKind;

typedef struct {
    const char *name, *verb, *argument;
    ActionKind kind;
} Action;

typedef enum { PRIMARY, INSPECT, HANDLE, USE, COMBINE, TOOLS, MOVE, ORDERS } ActionGroup;

unsigned story_actions(unsigned chapter, unsigned id, bool carried, ActionGroup group, Action list[MAX_ACTIONS]);
unsigned story_actions_fast(unsigned chapter, unsigned id, bool carried, ActionGroup group, Action list[MAX_ACTIONS]);
unsigned story_filter_defaults(unsigned id, Action list[MAX_ACTIONS], unsigned count);
// Unknown state keys fall back to the broader object mask, or all candidate verbs.
unsigned story_filter_defaults_masked(unsigned chapter, unsigned id, bool carried,
                                     Action list[MAX_ACTIONS], unsigned count);

// Incremental primary-menu filtering: show the static candidates immediately,
// then probe one candidate per call. The caller steps once per frame.
bool story_menu_filter_begin(unsigned chapter, unsigned id, bool carried,
                             Action list[MAX_ACTIONS], unsigned count);
bool story_menu_filter_step(void);
unsigned story_menu_filter_display(Action list[MAX_ACTIONS], unsigned capacity);
void story_menu_filter_cancel(void);
bool story_menu_filter_active(void);
void story_menu_probe_begin(void);
void story_menu_probe_end(void);
void story_menu_probe_set_budget(unsigned budget);
unsigned story_menu_probes_used(void);
unsigned story_menu_effect_probes_used(void);
void action_reply_normalized(const char *reply, char *out, unsigned capacity);
bool action_reply_is_no_effect(const char *reply);
bool action_command(Action action, unsigned object, const char *value, char line[COMMAND_SIZE]);
typedef enum { WORDS_SAY, WORDS_ANSWER, WORDS_COMMAND, WORDS_GREET } WordMode;
bool speech_command(WordMode mode, const char *phrase, char line[COMMAND_SIZE]);
void speech_word(unsigned index, char word[Z_WORD_MAX]);

#endif
