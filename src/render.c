#include "render.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Operator precedence: higher number = binds tighter. */
static int prec(expr_kind k)
{
    switch (k) {
    case EX_NUM: case EX_NAME: case EX_PLACEHOLDER:
    case EX_PAREN: case EX_ABS: case EX_SQRT: case EX_NTHROOT:
    case EX_FUNC: case EX_USERFUNC: case EX_DERIV: case EX_INTEG:
    case EX_SUM: case EX_WHERE: case EX_CMPLX:
        return 100;
    case EX_POW: return 80;
    case EX_NEG: return 70;
    case EX_MUL: case EX_IMUL: case EX_DIV: return 60;
    case EX_ADD: case EX_SUB: return 40;
    case EX_EQ:                return 10;
    }
    return 100;
}

/* Should the visual rendering wrap a child in parens for legibility? */
static int needs_parens(expr_kind par, int idx, expr_kind ch)
{
    int pp = prec(par), pc = prec(ch);
    switch (par) {
    case EX_DIV:                          /* numerator/denominator are visually grouped */
    case EX_PAREN:
    case EX_ABS:
    case EX_SQRT:
    case EX_NTHROOT:
    case EX_FUNC: case EX_USERFUNC:
    case EX_INTEG: case EX_SUM:
    case EX_DERIV: case EX_WHERE:
    case EX_CMPLX:
        return 0;
    case EX_POW:
        if (idx == 1) {
            /* exponent: HP parenthesizes additive/multiplicative children */
            if (ch == EX_ADD || ch == EX_SUB || ch == EX_NEG) return 1;
            return 0;
        }
        if (ch == EX_NEG) return 1;
        if (ch == EX_NUM || ch == EX_NAME) return 0;
        return pc < pp;
    case EX_NEG:
        return ch == EX_ADD || ch == EX_SUB;
    case EX_MUL: case EX_IMUL:
        if (ch == EX_ADD || ch == EX_SUB) return 1;
        if (idx == 1 && ch == EX_NEG) return 1;
        return 0;
    case EX_SUB:
        if (idx == 1 && (ch == EX_ADD || ch == EX_SUB)) return 1;
        return 0;
    case EX_ADD:
    case EX_EQ:
        return 0;
    case EX_NUM: case EX_NAME: case EX_PLACEHOLDER:
        return 0;
    }
    (void)pp;
    return 0;
}

/* Convenience: glyph from current font setting in ctx. */
static const glyph_t *gly(render_ctx *ctx, int ch)
{
    return ctx->use_mini ? font_mini(ch) : font_stack(ch);
}
static int FH(render_ctx *ctx) { return ctx->use_mini ? MINI_FONT_H : STACK_FONT_H; }
static int FAX(render_ctx *ctx) { return ctx->use_mini ? MINI_FONT_AXIS : STACK_FONT_AXIS; }

/* ------------- LAYOUT ------------- */

static void layout_text(render_ctx *ctx, expr *e)
{
    int w = 0;
    const char *s = e->text ? e->text : "";
    while (*s) {
        const glyph_t *g = gly(ctx, (unsigned char)*s++);
        w += g->advance;
    }
    if (w > 0) w -= 1; /* trim trailing advance gap */
    if (w < 1) w = 1;
    e->w = w;
    e->h = FH(ctx);
    e->axis = FAX(ctx);
}

static int paren_w_for(int h)
{
    return glyph_paren_width(h);
}

void render_layout(render_ctx *ctx, expr *e)
{
    if (!e) return;
    e->use_mini = ctx->use_mini;

    switch (e->kind) {

    case EX_NUM: case EX_NAME:
        if (!e->text || !e->text[0]) {
            /* "edit-mode placeholder": small filled block */
            e->w = 4; e->h = FH(ctx); e->axis = FAX(ctx);
        } else {
            layout_text(ctx, e);
        }
        break;

    case EX_PLACEHOLDER:
        e->w = 4; e->h = 4; e->axis = 2;
        break;

    case EX_NEG: {
        render_layout(ctx, e->kids[0]);
        int wp = needs_parens(EX_NEG, 0, e->kids[0]->kind) ? paren_w_for(e->kids[0]->h) * 2 : 0;
        e->w = 5 + 1 + e->kids[0]->w + wp; /* '-' + gap + body (+ parens) */
        e->h = e->kids[0]->h;
        e->axis = e->kids[0]->axis;
        if (e->axis < FAX(ctx)) e->axis = FAX(ctx);
        if (e->h < FH(ctx)) e->h = FH(ctx);
        break;
    }

    case EX_ADD: case EX_SUB: case EX_MUL: case EX_IMUL: case EX_EQ: {
        render_layout(ctx, e->kids[0]);
        render_layout(ctx, e->kids[1]);
        expr *a = e->kids[0], *b = e->kids[1];
        int aw = a->w, bw = b->w;
        int ah = a->h, bh = b->h;
        int aa = a->axis, ba = b->axis;
        int wpa = needs_parens(e->kind, 0, a->kind);
        int wpb = needs_parens(e->kind, 1, b->kind);
        int pa  = wpa ? 2 * paren_w_for(ah) : 0;
        int pb  = wpb ? 2 * paren_w_for(bh) : 0;
        int op_w = (e->kind == EX_IMUL || e->kind == EX_MUL) ? 3 : 5;
        e->w = aw + pa + 1 + op_w + 1 + bw + pb;
        int above = aa > ba ? aa : ba;
        int below_a = ah - aa, below_b = bh - ba;
        int below = below_a > below_b ? below_a : below_b;
        e->axis = above;
        e->h = above + below;
        if (e->h < FH(ctx)) { int d = FH(ctx) - e->h; e->h += d; }
        break;
    }

    case EX_DIV: {
        render_layout(ctx, e->kids[0]);
        render_layout(ctx, e->kids[1]);
        expr *n = e->kids[0], *d = e->kids[1];
        int nw = n->w, dw = d->w;
        e->w = (nw > dw ? nw : dw) + 2;
        /* HP layout: num + 1-row gap + bar + 1-row gap + den */
        e->h = n->h + 3 + d->h;
        e->axis = n->h + 1;   /* bar sits on parent axis (after gap) */
        break;
    }

    case EX_POW: {
        render_layout(ctx, e->kids[0]);
        int saved = ctx->use_mini; ctx->use_mini = 1;
        render_layout(ctx, e->kids[1]);
        ctx->use_mini = saved;
        expr *base = e->kids[0], *ex = e->kids[1];
        int wpb = needs_parens(EX_POW, 0, base->kind);
        int pb  = wpb ? 2 * paren_w_for(base->h) : 0;
        int raise = ex->h - 2; if (raise < 1) raise = 1;
        e->w = base->w + pb + 1 + ex->w;
        e->h = base->h + raise;
        e->axis = base->axis + raise;
        break;
    }

    case EX_PAREN: {
        render_layout(ctx, e->kids[0]);
        int pw = paren_w_for(e->kids[0]->h);
        e->w = e->kids[0]->w + 2 * pw;
        e->h = e->kids[0]->h;
        e->axis = e->kids[0]->axis;
        break;
    }

    case EX_ABS: {
        render_layout(ctx, e->kids[0]);
        e->w = e->kids[0]->w + 2 * 2;
        e->h = e->kids[0]->h;
        e->axis = e->kids[0]->axis;
        break;
    }

    case EX_SQRT: {
        render_layout(ctx, e->kids[0]);
        int hw = glyph_sqrt_hook_width(e->kids[0]->h);
        e->w = hw + e->kids[0]->w + 1;
        e->h = e->kids[0]->h + 2;
        e->axis = e->kids[0]->axis + 2;
        break;
    }

    case EX_NTHROOT: {
        int saved = ctx->use_mini; ctx->use_mini = 1;
        render_layout(ctx, e->kids[0]);  /* index in mini */
        ctx->use_mini = saved;
        render_layout(ctx, e->kids[1]);  /* body */
        expr *idx = e->kids[0], *body = e->kids[1];
        int hw = glyph_sqrt_hook_width(body->h);
        int hookleft = idx->w > hw - 1 ? idx->w - (hw - 1) : 0;
        e->w = hookleft + hw + body->w + 1;
        int idx_above = idx->h - 2; if (idx_above < 0) idx_above = 0;
        e->h = body->h + 2 + idx_above;
        e->axis = body->axis + 2 + idx_above;
        break;
    }

    case EX_FUNC: case EX_USERFUNC: {
        int name_w = font_measure_stack(e->text ? e->text : "");
        if (ctx->use_mini) name_w = font_measure_mini(e->text ? e->text : "");
        int max_h = FH(ctx), max_axis = FAX(ctx);
        int args_w = 0;
        for (int i = 0; i < e->nkids; i++) {
            render_layout(ctx, e->kids[i]);
            if (i) args_w += 1 + 5 + 1; /* comma + advance? actually just comma's advance is 6 */
            if (e->kids[i]->h > max_h) max_h = e->kids[i]->h;
            if (e->kids[i]->axis > max_axis) max_axis = e->kids[i]->axis;
            args_w += e->kids[i]->w;
        }
        int pw = paren_w_for(max_h);
        e->w = name_w + 2 * pw + args_w;
        e->h = max_h;
        e->axis = max_axis;
        if (max_h < FH(ctx)) e->h = FH(ctx);
        break;
    }

    case EX_DERIV: {
        /* ∂var (body) */
        render_layout(ctx, e->kids[0]);
        render_layout(ctx, e->kids[1]);
        int max_h = e->kids[0]->h > e->kids[1]->h ? e->kids[0]->h : e->kids[1]->h;
        int pw = paren_w_for(e->kids[1]->h);
        e->w = 5 + 1 + e->kids[0]->w + 1 + 2 * pw + e->kids[1]->w;
        e->h = max_h;
        e->axis = e->kids[1]->axis;
        if (e->h < FH(ctx)) e->h = FH(ctx);
        break;
    }

    case EX_INTEG: {
        /* lo, hi, body, var. Layout:
                hi
            (   |   ) body dvar
                |
                lo
           hi sits above the integral, lo below. The integral glyph spans
           a vertical region tall enough for the body. */
        int saved = ctx->use_mini; ctx->use_mini = 1;
        render_layout(ctx, e->kids[0]); /* lo */
        render_layout(ctx, e->kids[1]); /* hi */
        ctx->use_mini = saved;
        render_layout(ctx, e->kids[2]); /* body */
        render_layout(ctx, e->kids[3]); /* var */
        expr *lo = e->kids[0], *hi = e->kids[1], *body = e->kids[2], *var = e->kids[3];
        int lim_w = lo->w > hi->w ? lo->w : hi->w;
        int integ_glyph_h = body->h + 2;
        if (integ_glyph_h < 9) integ_glyph_h = 9;
        int int_w = glyph_int_width(integ_glyph_h);
        e->w = int_w + 1 + lim_w + 1 + body->w + 1 + 5 + 1 + var->w;
        e->h = hi->h + integ_glyph_h + lo->h;
        /* axis at vertical middle of integral glyph */
        e->axis = hi->h + integ_glyph_h / 2;
        break;
    }

    case EX_SUM: {
        int saved = ctx->use_mini; ctx->use_mini = 1;
        render_layout(ctx, e->kids[0]); /* var */
        render_layout(ctx, e->kids[1]); /* lo */
        render_layout(ctx, e->kids[2]); /* hi */
        ctx->use_mini = saved;
        render_layout(ctx, e->kids[3]); /* body */
        expr *var = e->kids[0], *lo = e->kids[1], *hi = e->kids[2], *body = e->kids[3];
        int low_w = var->w + 5 + lo->w; /* "var=lo" */
        int top_w = hi->w;
        int sub_w = low_w > top_w ? low_w : top_w;
        int sigma_w = glyph_sigma_width(0);
        if (sub_w < sigma_w) sub_w = sigma_w;
        int sigma_h = body->h + var->h + hi->h + 2;
        if (sigma_h < body->h + 6) sigma_h = body->h + 6;
        e->w = sub_w + 1 + body->w;
        e->h = sigma_h;
        e->axis = hi->h + 1 + body->axis;
        break;
    }

    case EX_WHERE: {
        for (int i = 0; i < 3; i++) render_layout(ctx, e->kids[i]);
        expr *body = e->kids[0], *v = e->kids[1], *val = e->kids[2];
        int max_h = body->h, max_a = body->axis;
        if (v->h > max_h) max_h = v->h;
        if (val->h > max_h) max_h = val->h;
        if (v->axis > max_a) max_a = v->axis;
        if (val->axis > max_a) max_a = val->axis;
        e->w = body->w + 1 + 2 + 1 + v->w + 5 + val->w;
        e->h = max_h;
        e->axis = max_a;
        break;
    }

    case EX_CMPLX: {
        render_layout(ctx, e->kids[0]);
        render_layout(ctx, e->kids[1]);
        expr *re = e->kids[0], *im = e->kids[1];
        int max_h = re->h > im->h ? re->h : im->h;
        int max_a = re->axis > im->axis ? re->axis : im->axis;
        int pw = paren_w_for(max_h);
        e->w = 2 * pw + re->w + 5 + im->w;
        e->h = max_h;
        e->axis = max_a;
        break;
    }
    }
}

/* ------------- PLACE & DRAW ------------- */

static void place_and_draw(render_ctx *ctx, bitmap_t *bm, expr *e, int x, int y);

static void draw_text(render_ctx *ctx, bitmap_t *bm, int x, int y, const char *s)
{
    if (ctx->use_mini) font_draw_mini(bm, x, y, s, 1);
    else font_draw_stack(bm, x, y, s, 1);
}

static void draw_glyph_at(render_ctx *ctx, bitmap_t *bm, int x, int y, int ch)
{
    const glyph_t *g = gly(ctx, ch);
    bm_blit_glyph(bm, x, y, g->rows, g->w, g->h, 1);
}

static int adv(render_ctx *ctx, int ch)
{
    return gly(ctx, ch)->advance;
}

static void place_and_draw(render_ctx *ctx, bitmap_t *bm, expr *e, int x, int y)
{
    if (!e) return;
    e->x = x; e->y = y;

    switch (e->kind) {

    case EX_NUM: case EX_NAME:
        if (e->text && e->text[0]) {
            draw_text(ctx, bm, x, y, e->text);
        } else {
            /* empty edit buffer: draw a small placeholder */
            bm_fill_rect(bm, x, y + 1, 4, e->h - 2, 1);
        }
        break;

    case EX_PLACEHOLDER:
        bm_fill_rect(bm, x, y, e->w, e->h, 1);
        break;

    case EX_NEG: {
        expr *a = e->kids[0];
        int ax = x;
        /* '-' aligned to axis */
        const glyph_t *g = font_stack('-');
        bm_blit_glyph(bm, ax, y + e->axis - g->axis, g->rows, g->w, g->h, 1);
        ax += 5 + 1;
        if (needs_parens(EX_NEG, 0, a->kind)) {
            int pw = paren_w_for(a->h);
            int paren_y = y + e->axis - a->axis;
            glyph_draw_paren_l(bm, ax, paren_y, a->h, 1);
            ax += pw;
            place_and_draw(ctx, bm, a, ax, paren_y);
            ax += a->w;
            glyph_draw_paren_r(bm, ax, paren_y, a->h, 1);
        } else {
            place_and_draw(ctx, bm, a, ax, y + e->axis - a->axis);
        }
        break;
    }

    case EX_ADD: case EX_SUB: case EX_MUL: case EX_IMUL: case EX_EQ: {
        expr *a = e->kids[0], *b = e->kids[1];
        int cx = x;
        int axis_y = y + e->axis;
        if (needs_parens(e->kind, 0, a->kind)) {
            int pw = paren_w_for(a->h);
            int py = axis_y - a->axis;
            glyph_draw_paren_l(bm, cx, py, a->h, 1);
            cx += pw;
            place_and_draw(ctx, bm, a, cx, py);
            cx += a->w;
            glyph_draw_paren_r(bm, cx, py, a->h, 1);
            cx += pw;
        } else {
            place_and_draw(ctx, bm, a, cx, axis_y - a->axis);
            cx += a->w;
        }
        cx += 1;
        int op_ch = '+';
        switch (e->kind) {
        case EX_ADD: op_ch = '+'; break;
        case EX_SUB: op_ch = '-'; break;
        case EX_MUL: op_ch = GL_DOT; break;
        case EX_IMUL: op_ch = GL_DOT; break;
        case EX_EQ:  op_ch = '='; break;
        default: break;
        }
        const glyph_t *gop = font_stack(op_ch);
        int op_y = axis_y - gop->axis;
        bm_blit_glyph(bm, cx, op_y, gop->rows, gop->w, gop->h, 1);
        int op_w = (e->kind == EX_MUL || e->kind == EX_IMUL) ? 3 : 5;
        cx += op_w + 1;
        if (needs_parens(e->kind, 1, b->kind)) {
            int pw = paren_w_for(b->h);
            int py = axis_y - b->axis;
            glyph_draw_paren_l(bm, cx, py, b->h, 1);
            cx += pw;
            place_and_draw(ctx, bm, b, cx, py);
            cx += b->w;
            glyph_draw_paren_r(bm, cx, py, b->h, 1);
        } else {
            place_and_draw(ctx, bm, b, cx, axis_y - b->axis);
        }
        break;
    }

    case EX_DIV: {
        expr *n = e->kids[0], *d = e->kids[1];
        int center = x + e->w / 2;
        int n_x = center - n->w / 2;
        int d_x = center - d->w / 2;
        place_and_draw(ctx, bm, n, n_x, y);
        /* gap row at y + n->h, bar at y + n->h + 1, gap at y + n->h + 2 */
        bm_hline(bm, x, y + n->h + 1, e->w, 1);
        place_and_draw(ctx, bm, d, d_x, y + n->h + 3);
        break;
    }

    case EX_POW: {
        expr *base = e->kids[0], *ex = e->kids[1];
        int raise = ex->h - 2; if (raise < 1) raise = 1;
        int base_y = y + raise;
        int cx = x;
        if (needs_parens(EX_POW, 0, base->kind)) {
            int pw = paren_w_for(base->h);
            glyph_draw_paren_l(bm, cx, base_y, base->h, 1);
            cx += pw;
            place_and_draw(ctx, bm, base, cx, base_y);
            cx += base->w;
            glyph_draw_paren_r(bm, cx, base_y, base->h, 1);
            cx += pw;
        } else {
            place_and_draw(ctx, bm, base, cx, base_y);
            cx += base->w;
        }
        cx += 1;
        int saved = ctx->use_mini; ctx->use_mini = 1;
        if (needs_parens(EX_POW, 1, ex->kind)) {
            int pw = paren_w_for(ex->h);
            glyph_draw_paren_l(bm, cx, y, ex->h, 1);
            cx += pw;
            place_and_draw(ctx, bm, ex, cx, y);
            cx += ex->w;
            glyph_draw_paren_r(bm, cx, y, ex->h, 1);
        } else {
            place_and_draw(ctx, bm, ex, cx, y);
        }
        ctx->use_mini = saved;
        break;
    }

    case EX_PAREN: {
        expr *body = e->kids[0];
        int pw = paren_w_for(body->h);
        glyph_draw_paren_l(bm, x, y, body->h, 1);
        place_and_draw(ctx, bm, body, x + pw, y);
        glyph_draw_paren_r(bm, x + pw + body->w, y, body->h, 1);
        break;
    }

    case EX_ABS: {
        expr *body = e->kids[0];
        glyph_draw_absbar(bm, x, y, body->h, 1);
        place_and_draw(ctx, bm, body, x + 2, y);
        glyph_draw_absbar(bm, x + 2 + body->w, y, body->h, 1);
        break;
    }

    case EX_SQRT: {
        expr *body = e->kids[0];
        int hw = glyph_sqrt_hook_width(body->h);
        glyph_draw_sqrt_hook(bm, x, y, body->h + 2, 1);
        bm_hline(bm, x + hw - 1, y, body->w + 2, 1);
        place_and_draw(ctx, bm, body, x + hw, y + 2);
        break;
    }

    case EX_NTHROOT: {
        expr *idx = e->kids[0], *body = e->kids[1];
        int hw = glyph_sqrt_hook_width(body->h);
        int idx_above = idx->h - 2; if (idx_above < 0) idx_above = 0;
        int hook_left = idx->w > hw - 1 ? idx->w - (hw - 1) : 0;
        int saved = ctx->use_mini; ctx->use_mini = 1;
        place_and_draw(ctx, bm, idx, x, y);
        ctx->use_mini = saved;
        glyph_draw_sqrt_hook(bm, x + hook_left, y + idx_above, body->h + 2, 1);
        bm_hline(bm, x + hook_left + hw - 1, y + idx_above, body->w + 2, 1);
        place_and_draw(ctx, bm, body, x + hook_left + hw, y + idx_above + 2);
        break;
    }

    case EX_FUNC: case EX_USERFUNC: {
        const char *name = e->text ? e->text : "";
        int name_w = ctx->use_mini ? font_measure_mini(name) : font_measure_stack(name);
        draw_text(ctx, bm, x, y + e->axis - FAX(ctx), name);
        int cx = x + name_w;
        int max_h = e->h;
        int pw = paren_w_for(max_h);
        glyph_draw_paren_l(bm, cx, y, max_h, 1);
        cx += pw;
        for (int i = 0; i < e->nkids; i++) {
            if (i) {
                draw_glyph_at(ctx, bm, cx, y + e->axis - FAX(ctx), ',');
                cx += adv(ctx, ',') + 1;
            }
            place_and_draw(ctx, bm, e->kids[i], cx, y + e->axis - e->kids[i]->axis);
            cx += e->kids[i]->w;
        }
        glyph_draw_paren_r(bm, cx, y, max_h, 1);
        break;
    }

    case EX_DERIV: {
        expr *var = e->kids[0], *body = e->kids[1];
        int cx = x;
        const glyph_t *gp = font_stack(GL_PARTIAL);
        int gp_y = y + e->axis - gp->axis;
        bm_blit_glyph(bm, cx, gp_y, gp->rows, gp->w, gp->h, 1);
        cx += gp->advance;
        place_and_draw(ctx, bm, var, cx, y + e->axis - var->axis);
        cx += var->w + 1;
        int pw = paren_w_for(body->h);
        glyph_draw_paren_l(bm, cx, y + e->axis - body->axis, body->h, 1);
        cx += pw;
        place_and_draw(ctx, bm, body, cx, y + e->axis - body->axis);
        cx += body->w;
        glyph_draw_paren_r(bm, cx, y + e->axis - body->axis, body->h, 1);
        break;
    }

    case EX_INTEG: {
        expr *lo = e->kids[0], *hi = e->kids[1], *body = e->kids[2], *var = e->kids[3];
        int integ_glyph_h = body->h + 2;
        if (integ_glyph_h < 9) integ_glyph_h = 9;
        int int_w = glyph_int_width(integ_glyph_h);
        int integ_y = y + hi->h;
        glyph_draw_integral(bm, x, integ_y, integ_glyph_h, 1);
        int saved = ctx->use_mini; ctx->use_mini = 1;
        /* hi sits above the integral, lo below, both indented to the right. */
        place_and_draw(ctx, bm, hi, x + 1, y);
        place_and_draw(ctx, bm, lo, x, y + hi->h + integ_glyph_h);
        ctx->use_mini = saved;
        int lim_w = lo->w > hi->w ? lo->w : hi->w;
        int cx = x + int_w + 1 + lim_w + 1;
        place_and_draw(ctx, bm, body, cx, y + e->axis - body->axis);
        cx += body->w + 1;
        const glyph_t *gd = font_stack('d');
        bm_blit_glyph(bm, cx, y + e->axis - gd->axis, gd->rows, gd->w, gd->h, 1);
        cx += gd->advance;
        place_and_draw(ctx, bm, var, cx, y + e->axis - var->axis);
        break;
    }

    case EX_SUM: {
        expr *var = e->kids[0], *lo = e->kids[1], *hi = e->kids[2], *body = e->kids[3];
        int sigma_h = e->h;
        int sigma_w = glyph_sigma_width(sigma_h);
        int low_w = var->w + 5 + lo->w;
        int top_w = hi->w;
        int sub_w = low_w > top_w ? low_w : top_w;
        if (sub_w < sigma_w) sub_w = sigma_w;
        /* hi at top */
        int saved = ctx->use_mini; ctx->use_mini = 1;
        place_and_draw(ctx, bm, hi, x + (sub_w - hi->w) / 2, y);
        /* sigma below */
        int sigma_y = y + hi->h + 1;
        glyph_draw_sigma(bm, x + (sub_w - sigma_w) / 2, sigma_y, sigma_h - hi->h - 1 - var->h - 1, 1);
        /* var=lo at bottom */
        int low_x = x + (sub_w - low_w) / 2;
        place_and_draw(ctx, bm, var, low_x, y + sigma_h - var->h);
        const glyph_t *geq = font_mini('=');
        bm_blit_glyph(bm, low_x + var->w + 1, y + sigma_h - var->h + var->axis - geq->axis,
                      geq->rows, geq->w, geq->h, 1);
        place_and_draw(ctx, bm, lo, low_x + var->w + 5, y + sigma_h - lo->h);
        ctx->use_mini = saved;
        /* body to right */
        place_and_draw(ctx, bm, body, x + sub_w + 1, y + e->axis - body->axis);
        break;
    }

    case EX_WHERE: {
        expr *body = e->kids[0], *v = e->kids[1], *val = e->kids[2];
        int cx = x;
        place_and_draw(ctx, bm, body, cx, y + e->axis - body->axis);
        cx += body->w + 1;
        glyph_draw_absbar(bm, cx, y, e->h, 1);
        cx += 2 + 1;
        place_and_draw(ctx, bm, v, cx, y + e->axis - v->axis);
        cx += v->w;
        const glyph_t *gop = font_stack('=');
        bm_blit_glyph(bm, cx, y + e->axis - gop->axis, gop->rows, gop->w, gop->h, 1);
        cx += gop->advance;
        place_and_draw(ctx, bm, val, cx, y + e->axis - val->axis);
        break;
    }

    case EX_CMPLX: {
        expr *re = e->kids[0], *im = e->kids[1];
        int pw = paren_w_for(e->h);
        glyph_draw_paren_l(bm, x, y, e->h, 1);
        place_and_draw(ctx, bm, re, x + pw, y + e->axis - re->axis);
        const glyph_t *gc = font_stack(',');
        bm_blit_glyph(bm, x + pw + re->w, y + e->axis - gc->axis, gc->rows, gc->w, gc->h, 1);
        place_and_draw(ctx, bm, im, x + pw + re->w + 5, y + e->axis - im->axis);
        glyph_draw_paren_r(bm, x + e->w - pw, y, e->h, 1);
        break;
    }
    }
}

void render_eqw(render_ctx *ctx, bitmap_t *bm, expr *root)
{
    if (!root) return;
    int saved_mini = ctx->use_mini;
    render_layout(ctx, root);
    ctx->use_mini = saved_mini;
    int x = (bm->w - root->w) / 2;
    int y = (bm->h - root->h) / 2;
    if (x < 1) x = 1;
    if (y < 0) y = 0;
    if (x + root->w > bm->w - 1) x = bm->w - 1 - root->w;
    if (x < 0) x = 0;
    place_and_draw(ctx, bm, root, x, y);

    /* selection overlay: a 1-px box around the selected sub-expression with
       a 1-px gap inside so it doesn't crowd the glyphs (matches the boxed
       selection style shown in Meta Kernel §8.1.2 / §8.8.1). */
    if (ctx->sel) {
        int bx = ctx->sel->x - 2;
        int by = ctx->sel->y - 1;
        int bw = ctx->sel->w + 4;
        int bh = ctx->sel->h + 2;
        if (bx < 0) { bw += bx; bx = 0; }
        if (by < 0) { bh += by; by = 0; }
        if (bx + bw > bm->w) bw = bm->w - bx;
        if (by + bh > bm->h) bh = bm->h - by;
        bm_rect(bm, bx, by, bw, bh, 1);
    }

    /* edit caret */
    if (ctx->edit && ctx->caret_visible) {
        int cx = ctx->edit->x + ctx->edit->w + 1;
        int cy = ctx->edit->y;
        const glyph_t *g = font_stack(GL_CARET_OPEN);
        bm_blit_glyph(bm, cx, cy, g->rows, g->w, g->h, 1);
    }

    /* cursor mode caret */
    if (ctx->cursor_mode) {
        bm_fill_rect(bm, ctx->cursor_x, ctx->cursor_y, 3, 3, 1);
    }
}

void render_layout_only(render_ctx *ctx, expr *e) { render_layout(ctx, e); }

/* Hit test: find smallest containing node. */
expr *render_hit(expr *root, int x, int y)
{
    if (!root) return NULL;
    if (x < root->x || y < root->y || x >= root->x + root->w || y >= root->y + root->h)
        return NULL;
    for (int i = 0; i < root->nkids; i++) {
        expr *h = render_hit(root->kids[i], x, y);
        if (h) return h;
    }
    return root;
}
