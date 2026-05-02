#include "bitmap.h"
#include <stdlib.h>
#include <string.h>

bitmap_t *bm_new(int w, int h)
{
    bitmap_t *bm = calloc(1, sizeof(*bm));
    bm->w = w;
    bm->h = h;
    bm->stride = (w + 7) / 8;
    bm->bits = calloc((size_t)bm->stride * h, 1);
    bm->owned = 1;
    return bm;
}

void bm_free(bitmap_t *bm)
{
    if (!bm) return;
    if (bm->owned) free(bm->bits);
    free(bm);
}

void bm_clear(bitmap_t *bm)
{
    memset(bm->bits, 0, (size_t)bm->stride * bm->h);
}

void bm_set(bitmap_t *bm, int x, int y, int v)
{
    if (x < 0 || y < 0 || x >= bm->w || y >= bm->h) return;
    uint8_t *p = &bm->bits[y * bm->stride + (x >> 3)];
    uint8_t m = (uint8_t)(1u << (7 - (x & 7)));
    if (v) *p |= m;
    else   *p &= (uint8_t)~m;
}

int bm_get(const bitmap_t *bm, int x, int y)
{
    if (x < 0 || y < 0 || x >= bm->w || y >= bm->h) return 0;
    return (bm->bits[y * bm->stride + (x >> 3)] >> (7 - (x & 7))) & 1;
}

void bm_xor(bitmap_t *bm, int x, int y)
{
    if (x < 0 || y < 0 || x >= bm->w || y >= bm->h) return;
    bm->bits[y * bm->stride + (x >> 3)] ^= (uint8_t)(1u << (7 - (x & 7)));
}

void bm_hline(bitmap_t *bm, int x, int y, int len, int v)
{
    for (int i = 0; i < len; i++) bm_set(bm, x + i, y, v);
}

void bm_vline(bitmap_t *bm, int x, int y, int len, int v)
{
    for (int i = 0; i < len; i++) bm_set(bm, x, y + i, v);
}

void bm_fill_rect(bitmap_t *bm, int x, int y, int w, int h, int v)
{
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) bm_set(bm, x + i, y + j, v);
}

void bm_invert_rect(bitmap_t *bm, int x, int y, int w, int h)
{
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) bm_xor(bm, x + i, y + j);
}

void bm_rect(bitmap_t *bm, int x, int y, int w, int h, int v)
{
    if (w <= 0 || h <= 0) return;
    bm_hline(bm, x, y, w, v);
    bm_hline(bm, x, y + h - 1, w, v);
    bm_vline(bm, x, y, h, v);
    bm_vline(bm, x + w - 1, y, h, v);
}

void bm_blit_glyph(bitmap_t *bm, int x, int y,
                   const uint8_t *rows, int w, int h, int v)
{
    for (int j = 0; j < h; j++) {
        uint8_t r = rows[j];
        for (int i = 0; i < w; i++)
            if (r & (1u << i)) bm_set(bm, x + i, y + j, v);
    }
}

void bm_blit_glyph_clipped(bitmap_t *bm, int x, int y, int cx, int cy, int cw, int ch,
                           const uint8_t *rows, int w, int h, int v)
{
    for (int j = 0; j < h; j++) {
        uint8_t r = rows[j];
        for (int i = 0; i < w; i++) {
            if (!(r & (1u << i))) continue;
            int px = x + i, py = y + j;
            if (px < cx || py < cy || px >= cx + cw || py >= cy + ch) continue;
            bm_set(bm, px, py, v);
        }
    }
}
