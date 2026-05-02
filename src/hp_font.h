#ifndef SDLEQW_HP_FONT_H
#define SDLEQW_HP_FONT_H

#include <stdint.h>

typedef struct {
    uint8_t w;          /* width in pixels */
    uint8_t advance;    /* x advance (= w + 1 typically) */
    const uint8_t *rows;/* 7 row bytes, low bit = leftmost pixel */
} hp_glyph_t;

extern const hp_glyph_t hp_glyph_table[256];

#endif
