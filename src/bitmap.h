#ifndef SDLEQW_BITMAP_H
#define SDLEQW_BITMAP_H

#include <stdint.h>

typedef struct {
    int w, h;
    int stride;
    uint8_t *bits;
    int owned;
} bitmap_t;

bitmap_t *bm_new(int w, int h);
void      bm_free(bitmap_t *bm);
void      bm_clear(bitmap_t *bm);
void      bm_set(bitmap_t *bm, int x, int y, int v);
int       bm_get(const bitmap_t *bm, int x, int y);
void      bm_xor(bitmap_t *bm, int x, int y);

void bm_hline(bitmap_t *bm, int x, int y, int len, int v);
void bm_vline(bitmap_t *bm, int x, int y, int len, int v);
void bm_rect(bitmap_t *bm, int x, int y, int w, int h, int v);
void bm_fill_rect(bitmap_t *bm, int x, int y, int w, int h, int v);
void bm_invert_rect(bitmap_t *bm, int x, int y, int w, int h);

/* Bit 0 of each row byte = leftmost pixel. One byte per row when w<=8. */
void bm_blit_glyph(bitmap_t *bm, int x, int y,
                   const uint8_t *rows, int w, int h, int v);
void bm_blit_glyph_clipped(bitmap_t *bm, int x, int y, int cx, int cy, int cw, int ch,
                           const uint8_t *rows, int w, int h, int v);

#endif
