#ifndef ZORK_TRANSCRIPT_CODEC_H
#define ZORK_TRANSCRIPT_CODEC_H

#include <stdbool.h>
#include <stdint.h>

unsigned transcript_codec_encoded_size(const char *text, unsigned length);
bool transcript_codec_encode(const char *text, unsigned length, uint8_t *out, unsigned capacity,
                             unsigned *encoded_size);
bool transcript_codec_expand(const uint8_t *encoded, unsigned length, char *out, unsigned capacity,
                             unsigned *expanded_size);

#endif
