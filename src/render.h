#ifndef SDLEQW_RENDER_H
#define SDLEQW_RENDER_H

#include "bitmap.h"
#include "expr.h"
#include "font.h"

typedef struct {
    int   use_mini;
    expr *sel;            /* selection (NULL = none) */
    int   sel_boxed;      /* 0 = inverted, 1 = boxed */
    expr *edit;           /* the node we're editing (text caret displayed at end) */
    int   caret_visible;  /* blink phase */
    /* cursor mode (free-roaming caret) */
    int   cursor_mode;
    int   cursor_x;
    int   cursor_y;
} render_ctx;

void render_layout (render_ctx *ctx, expr *e);
void render_place  (render_ctx *ctx, expr *e, int x, int y);
void render_draw   (render_ctx *ctx, bitmap_t *bm, expr *root,
                    int *out_x, int *out_y);

/* Convenience top-level: layout + center + draw + selection/caret overlays. */
void render_eqw    (render_ctx *ctx, bitmap_t *bm, expr *root);

/* Traverse the tree and find the deepest node whose bounding box contains (x, y). */
expr *render_hit   (expr *root, int x, int y);

#endif
