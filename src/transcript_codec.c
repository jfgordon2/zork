#include "transcript_codec.h"

enum { ESCAPE = 0x1b, TOKEN_BASE = 0x80 };

// Shared 128-entry byte-pair vocabulary trained on complete Zork I–III playthroughs.
// Frequent subword fragments are intentional; freeze this mapping once cartridge saves ship.
static const char *const tokens[] = {
    "e ", "th", "s ", "in", " a", "er", " th", "  ", "ou", "st", "d ", " the ",
    "is ", "or", "> ", "en", "ar", "on", "t ", "\nT", "an", "ro", "of", ". ",
    "ea", "ll", " s", "ing", "om", "\nTh", "y ", "to", " b", "ed ", " w", " an",
    "as", "it", " c", "    ", "is a", "lo", " a ", "es", "ch", " h", "at", "all",
    "est", "e.", "ere ", "es ", "id", "\nThe ", "ak", "el", "you", "ed", ", ",
    "ing ", "orth", "outh", "ck", "ow", " and ", "You", "north", "re", "ag", "..",
    ". T", "of the ", "is a ", "li", "south", "\nYou", "re ", "Ro", "east", "ass",
    "Room", "ol", " is ", "ent", " d", "ra", " f", "et", "room", "wa", "gh", "west",
    "la", "\n    ", ". Th", "al", " p", "e p", "ith", "ri", "of ", "\nThere ", "im", "ear",
    "to the ", "bl", "un", "ver", "ere.", "hi", "bo", "op", ".\n", "the ", "ge ",
    " are ", " in", "ha", "ic", "ve ", "ur", "oor", "pp", "ir", "  +", "ta", "rea", "se"
};
#define TOKEN_COUNT (sizeof(tokens) / sizeof(tokens[0]))
_Static_assert(TOKEN_COUNT == 128, "transcript token IDs use all 128 high-byte values");

static unsigned token_match(const char *text, unsigned pos, unsigned end, unsigned *token) {
    unsigned best = 0;
    for (unsigned i = 0; i < TOKEN_COUNT; i++) {
        const char *pattern = tokens[i];
        unsigned length = 0;
        while (pattern[length] && pos + length < end && text[pos + length] == pattern[length]) length++;
        if (!pattern[length] && length > best) {
            best = length;
            *token = i;
        }
    }
    return best;
}

unsigned transcript_codec_encoded_size(const char *text, unsigned length) {
    unsigned size = 0;
    for (unsigned pos = 0; pos < length;) {
        unsigned token = 0;
        unsigned match = token_match(text, pos, length, &token);
        if (match) {
            size++;
            pos += match;
        } else {
            uint8_t value = (uint8_t)text[pos++];
            size += value == ESCAPE || value >= TOKEN_BASE ? 2 : 1;
        }
    }
    return size;
}

bool transcript_codec_encode(const char *text, unsigned length, uint8_t *out, unsigned capacity,
                             unsigned *encoded_size) {
    unsigned needed = transcript_codec_encoded_size(text, length);
    if (!out || !encoded_size || needed > capacity) return false;
    unsigned size = 0;
    for (unsigned pos = 0; pos < length;) {
        unsigned token = 0;
        unsigned match = token_match(text, pos, length, &token);
        if (match) {
            out[size++] = TOKEN_BASE + token;
            pos += match;
        } else {
            uint8_t value = (uint8_t)text[pos++];
            if (value == ESCAPE || value >= TOKEN_BASE) out[size++] = ESCAPE;
            out[size++] = value;
        }
    }
    *encoded_size = size;
    return true;
}

bool transcript_codec_expand(const uint8_t *encoded, unsigned length, char *out, unsigned capacity,
                             unsigned *expanded_size) {
    if (!encoded || !out || !expanded_size) return false;
    unsigned used = 0;
    for (unsigned pos = 0; pos < length;) {
        uint8_t value = encoded[pos++];
        if (value == ESCAPE) {
            if (pos == length || used >= capacity) return false;
            out[used++] = (char)encoded[pos++];
        } else if (value >= TOKEN_BASE) {
            unsigned token = value - TOKEN_BASE;
            if (token >= TOKEN_COUNT) return false;
            const char *pattern = tokens[token];
            for (unsigned i = 0; pattern[i]; i++) {
                if (used >= capacity) return false;
                out[used++] = pattern[i];
            }
        } else {
            if (used >= capacity) return false;
            out[used++] = (char)value;
        }
    }
    if (used >= capacity) return false;
    out[used] = 0;
    *expanded_size = used;
    return true;
}
