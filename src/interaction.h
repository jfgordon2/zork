#ifndef ZORK_INTERACTION_H
#define ZORK_INTERACTION_H

#include "chapter.h"

enum { MAX_ACTIONS = 18, COMMAND_SIZE = 128, WORDS_PER_PHRASE = 3 };

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
unsigned story_filter_defaults(unsigned id, Action list[MAX_ACTIONS], unsigned count);
void story_menu_probe_begin(void);
void story_menu_probe_end(void);
void action_reply_normalized(const char *reply, char *out, unsigned capacity);
bool action_reply_is_no_effect(const char *reply);
bool action_command(Action action, unsigned object, const char *value, char line[COMMAND_SIZE]);
bool speech_command(unsigned mode, const char *phrase, char line[COMMAND_SIZE]);
void speech_word(unsigned index, char word[Z_WORD_MAX]);

#endif
