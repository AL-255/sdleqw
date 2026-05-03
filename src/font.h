#ifndef SDLEQW_FONT_H
#define SDLEQW_FONT_H

#include "bitmap.h"

typedef struct {
    int           w, h;
    int           advance;       /* pixels to advance the pen after this glyph */
    int           baseline;      /* y of typographic baseline (rows below = descent) */
    int           axis;          /* y of math axis (vertical center used for alignment) */
    const uint8_t *rows;
} glyph_t;

/* Codepoints we recognize beyond ASCII. We re-use slots 128.. as a private
   subset for math symbols. The eqw renderer references them by name via
   GLYPH_* macros below. */
enum {
    GL_DOT       = 128, /* centered multiplication dot */
    GL_PI        = 129,
    GL_THETA     = 130,
    GL_LAMBDA    = 131,
    GL_MU        = 132,
    GL_DELTA     = 133, /* uppercase Delta */
    GL_EPSILON   = 134,
    GL_INFINITY  = 135,
    GL_PARTIAL   = 136, /* 'd' style partial sign */
    GL_LEQ       = 137,
    GL_GEQ       = 138,
    GL_NEQ       = 139,
    GL_DEGREE    = 140,
    GL_PHI       = 141,
    GL_OMEGA     = 142,
    GL_ALPHA     = 143,
    GL_BETA      = 144,
    GL_GAMMA     = 145,
    GL_RIGHTARROW= 146, /* small ascii-style => arrow used for context indicators */
    GL_BLOCK     = 147, /* solid block (selection placeholder, edit caret) */
    GL_CARET_OPEN= 148, /* edit caret (right-pointing triangle) */
    GL_CARET_CLOSE=149, /* shifted edit caret */
    GL_END       = 150,
};

/* Stack font (5x7) and mini font (4x6). */
const glyph_t *font_stack(int ch);
const glyph_t *font_mini(int ch);

/* Convenience text drawing in stack/mini fonts. Returns x advance. */
int font_draw_stack(bitmap_t *bm, int x, int y, const char *s, int v);
int font_draw_mini (bitmap_t *bm, int x, int y, const char *s, int v);

/* Measure pixel width (with inter-char advance) and intrinsic height. */
int font_measure_stack(const char *s);
int font_measure_mini (const char *s);

/* HP advance rule: column step from a glyph of width prev_w to next of
   width next_w (next_w == 0 means no following glyph). */
int font_hp_advance(int prev_w, int next_w);

/* Heights as constants for layout. */
#define STACK_FONT_H 7
#define MINI_FONT_H  6
#define STACK_FONT_AXIS 3   /* visual center row */
#define MINI_FONT_AXIS  2

/* Procedural tall glyphs that scale with content. */
void glyph_draw_integral(bitmap_t *bm, int x, int y_top, int height, int v);
void glyph_draw_sigma   (bitmap_t *bm, int x, int y_top, int height, int v);
void glyph_draw_paren_l (bitmap_t *bm, int x, int y_top, int height, int v);
void glyph_draw_paren_r (bitmap_t *bm, int x, int y_top, int height, int v);
void glyph_draw_absbar  (bitmap_t *bm, int x, int y_top, int height, int v);
void glyph_draw_sqrt_hook(bitmap_t *bm, int x, int y_top, int height, int v);

int  glyph_int_width   (int height);
int  glyph_sigma_width (int height);
int  glyph_paren_width (int height);
int  glyph_absbar_width(int height);
int  glyph_sqrt_hook_width(int height);

#endif
