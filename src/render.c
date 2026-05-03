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

/* Trailing pad: how much extra cursor-reserve column space HP places to the
   RIGHT of the visible content for the EDIT cursor. Depends on the rightmost
   glyph in the expression. = max(g.w + 1, 5) - g.w. */
static int trailing_pad(const expr *e)
{
    if (!e) return 0;
    switch (e->kind) {
    case EX_NUM:
    case EX_NAME: {
        if (!e->text || !e->text[0]) return 0;
        char last = e->text[strlen(e->text) - 1];
        const glyph_t *g = font_stack((unsigned char)last);
        int adv = g->w + 1;
        if (adv < 5) adv = 5;
        return adv - g->w;
    }
    case EX_ADD: case EX_SUB: case EX_MUL: case EX_IMUL: case EX_EQ:
        return trailing_pad(e->kids[1]);
    case EX_POW:
        return trailing_pad(e->kids[1]);
    case EX_DIV:
        return 0;
    case EX_NEG:
        return trailing_pad(e->kids[0]);
    case EX_PAREN: case EX_ABS:
    case EX_SQRT: case EX_NTHROOT:
    case EX_FUNC: case EX_USERFUNC:
    case EX_CMPLX:
        return 0;  /* ends with a closing bracket / right cap */
    case EX_INTEG:
        if (e->nkids >= 4) return trailing_pad(e->kids[3]);  /* var */
        return 0;
    case EX_SUM:
        if (e->nkids >= 4) return trailing_pad(e->kids[3]);  /* body */
        return 0;
    case EX_DERIV:
        if (e->nkids >= 2) return 0;  /* ends with paren */
        return 0;
    case EX_WHERE:
        if (e->nkids >= 3) return trailing_pad(e->kids[2]);
        return 0;
    case EX_PLACEHOLDER:
        return 0;
    }
    return 0;
}

/* Width of the visually leftmost atomic glyph in expr e (for inter-operand
   advance gaps).  Falls back to e->w when the leftmost token isn't a single
   glyph (e.g. a sub-expression with parens / vinculum on the outside). */
static int first_glyph_w(const expr *e)
{
    if (!e) return 0;
    switch (e->kind) {
    case EX_NUM:
    case EX_NAME:
        if (e->text && e->text[0])
            return font_stack((unsigned char)e->text[0])->w;
        return e->w;
    case EX_ADD: case EX_SUB: case EX_MUL: case EX_IMUL: case EX_EQ:
        return first_glyph_w(e->kids[0]);
    case EX_POW:
        return first_glyph_w(e->kids[0]);
    case EX_NEG:
        return 5;  /* leading '-' */
    case EX_FUNC: case EX_USERFUNC:
        if (e->text && e->text[0])
            return font_stack((unsigned char)e->text[0])->w;
        return 5;
    default:
        return e->w;
    }
}

/* Width of the rightmost atomic glyph in expr e. */
static int last_glyph_w(const expr *e)
{
    if (!e) return 0;
    switch (e->kind) {
    case EX_NUM:
    case EX_NAME:
        if (e->text && e->text[0])
            return font_stack((unsigned char)e->text[strlen(e->text) - 1])->w;
        return e->w;
    case EX_ADD: case EX_SUB: case EX_MUL: case EX_IMUL: case EX_EQ:
        return last_glyph_w(e->kids[1]);
    case EX_POW:
        return last_glyph_w(e->kids[1]);
    case EX_NEG:
        return last_glyph_w(e->kids[0]);
    default:
        return e->w;
    }
}

/* ------------- LAYOUT ------------- */

static void layout_text(render_ctx *ctx, expr *e)
{
    /* Use the same advance formula as font_draw_stack/font_measure_stack so
       the rendered width matches HP firmware. */
    int w = 0;
    if (ctx->use_mini) {
        w = font_measure_mini(e->text ? e->text : "");
    } else {
        w = font_measure_stack(e->text ? e->text : "");
    }
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
        /* HP reserves full stack-font extent for an empty placeholder so
           surrounding parens/sqrt scale to a normal-line height. */
        e->w = 5; e->h = STACK_FONT_H; e->axis = STACK_FONT_AXIS;
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
        int op_w = (e->kind == EX_IMUL || e->kind == EX_MUL) ? 1 : 5;
        /* Inter-operand gap.  For +/- and = operators, HP uses
           font_hp_advance(last_glyph_w(a), op_w) so a narrow trailing
           glyph (like the "I" in "PI") gets the +1 gap.  Multiplication
           dot uses gap_a=1, gap_b=2 (HP places one extra col after the
           cdot before the next operand). */
        int last_a_w  = wpa ? 5 : last_glyph_w(a);
        int first_b_w = wpb ? 5 : first_glyph_w(b);
        int gap_a, gap_b;
        if (op_w == 1) {
            /* MUL cdot: gap_a = 1 always; gap_b = 1, +1 when next operand
               starts with a narrow glyph (w < 4) — same +1-narrow bonus
               as the rest of HP's spacing. */
            gap_a = 1;
            gap_b = 1 + (first_b_w > 0 && first_b_w < 4 ? 1 : 0);
        } else {
            gap_a = font_hp_advance(last_a_w, op_w) - last_a_w;
            gap_b = font_hp_advance(op_w, first_b_w) - op_w;
            if (gap_a < 1) gap_a = 1;
            if (gap_b < 1) gap_b = 1;
        }
        e->w = aw + pa + gap_a + op_w + gap_b + bw + pb;
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
        /* HP bar width:
           - At least num.w + 2 (1-col padding each side)
           - At least den.w + 9 when den is the *currently-edited* atom
             wider than 5, to fit the cursor reserve to its right.  The
             reserve only applies while editing; navigating away (SEL
             mode) collapses the bar to its natural width.  For structured
             dens (DIV/PAREN/etc.) the cursor lives INSIDE so no extra
             reserve.
           - Minimum 13 cols. */
        int nw = n->w + 2;
        int dw = d->w + 2;
        int den_atom = (d->kind == EX_NUM || d->kind == EX_NAME);
        int den_edited = ctx && ctx->edit == d;
        if (den_atom && d->w > 5 && den_edited) dw = d->w + 9;
        int content = nw > dw ? nw : dw;
        e->w = content > 13 ? content : 13;
        /* HP layout: num + 1-row gap + bar + 1-row gap + den */
        e->h = n->h + 3 + d->h;
        e->axis = n->h + 1;
        break;
    }

    case EX_POW: {
        render_layout(ctx, e->kids[0]);
        render_layout(ctx, e->kids[1]);
        expr *base = e->kids[0], *ex = e->kids[1];
        int wpb = needs_parens(EX_POW, 0, base->kind);
        int pb  = wpb ? 2 * paren_w_for(base->h) : 0;
        int wpe = needs_parens(EX_POW, 1, ex->kind);
        int pe  = wpe ? 2 * paren_w_for(ex->h) : 0;
        int last_base_w = wpb ? 5 : last_glyph_w(base);
        int first_ex_w  = wpe ? 5 : first_glyph_w(ex);
        int gap = font_hp_advance(last_base_w, first_ex_w) - last_base_w;
        if (gap < 1) gap = 1;
        int raise = ex->h - 1;
        e->w = base->w + pb + gap + pe + ex->w;
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
        /* HP geometry: tail(2) + vertical(1) + left_gap(1) + body + right_pad
           where right_pad depends on body kind: 8 cols for atomic / linear
           bodies (cursor reserve), 2 cols for structured bodies whose own
           cursor lives inside (DIV, INTEG, SUM, etc.). */
        int hw = glyph_sqrt_hook_width(e->kids[0]->h);
        int rpad;
        switch (e->kids[0]->kind) {
        case EX_DIV: case EX_INTEG: case EX_SUM:
        case EX_DERIV: case EX_WHERE:
            rpad = 2; break;
        default:
            rpad = 8; break;
        }
        e->w = hw + 1 + e->kids[0]->w + rpad;
        e->h = e->kids[0]->h + 3;
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
            if (i) args_w += 1 + 5 + 1; /* comma + advance */
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
        e->axis = hi->h + integ_glyph_h / 2;
        break;
    }

    case EX_SUM: {
        /* HP renders sum's var/lo/hi at full stack-font size. */
        render_layout(ctx, e->kids[0]); /* var */
        render_layout(ctx, e->kids[1]); /* lo */
        render_layout(ctx, e->kids[2]); /* hi */
        render_layout(ctx, e->kids[3]); /* body */
        expr *var = e->kids[0], *lo = e->kids[1], *hi = e->kids[2], *body = e->kids[3];
        /* Compute "var=lo" width using HP advance rules.  For atomic
           var/lo we measure the concatenated string; otherwise approximate. */
        int low_w;
        if ((var->kind == EX_NUM || var->kind == EX_NAME) &&
            (lo->kind == EX_NUM || lo->kind == EX_NAME)) {
            char buf[80];
            snprintf(buf, sizeof(buf), "%s=%s",
                     var->text ? var->text : "",
                     lo->text ? lo->text : "");
            low_w = font_measure_stack(buf);
        } else {
            low_w = var->w + 7 + lo->w;
        }
        int top_w = hi->w;
        /* HP: sigma is square, height = body.h + 6. */
        int sigma_h = body->h + 6;
        if (sigma_h < 13) sigma_h = 13;
        int sigma_w = sigma_h;
        int sub_w = low_w > top_w ? low_w : top_w;
        if (sub_w < sigma_w) sub_w = sigma_w;
        /* HP: 3 cols of gap between sigma's right edge and body. */
        e->w = sub_w + 3 + body->w;
        /* HP layout: hi + 1-row gap + sigma + 1-row gap + var=lo */
        e->h = hi->h + 1 + sigma_h + 1 + var->h;
        e->axis = hi->h + 1 + sigma_h / 2;
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
        /* HP shows no visible placeholder rect — just reserves layout width.
           The blinking edit caret (if visible) takes care of indicating it. */
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
        int op_w = (e->kind == EX_MUL || e->kind == EX_IMUL) ? 1 : 5;
        int wpa = needs_parens(e->kind, 0, a->kind);
        int wpb = needs_parens(e->kind, 1, b->kind);
        int last_a_w  = wpa ? 5 : last_glyph_w(a);
        int first_b_w = wpb ? 5 : first_glyph_w(b);
        int gap_a, gap_b;
        if (op_w == 1) {
            gap_a = 1;
            gap_b = 1 + (first_b_w > 0 && first_b_w < 4 ? 1 : 0);
        } else {
            gap_a = font_hp_advance(last_a_w, op_w) - last_a_w;
            gap_b = font_hp_advance(op_w, first_b_w) - op_w;
            if (gap_a < 1) gap_a = 1;
            if (gap_b < 1) gap_b = 1;
        }
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
        cx += gap_a;
        if (e->kind != EX_IMUL) {
            int op_ch = '+';
            switch (e->kind) {
            case EX_ADD: op_ch = '+'; break;
            case EX_SUB: op_ch = '-'; break;
            case EX_MUL: op_ch = GL_DOT; break;
            case EX_EQ:  op_ch = '='; break;
            default: break;
            }
            const glyph_t *gop = font_stack(op_ch);
            int op_y = axis_y - gop->axis;
            bm_blit_glyph(bm, cx, op_y, gop->rows, gop->w, gop->h, 1);
        }
        cx += op_w + gap_b;
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
        /* HP centers numerator's visual width on the bar.  For atomic
           NUM/NAME, HP rounds *toward the wider end of the glyph string*
           so that the visual mass sits near the bar center.  For
           structured nums HP floor-aligns. */
        int n_off;
        if ((n->kind == EX_NUM || n->kind == EX_NAME) && n->text && n->text[0]) {
            int round_up = 0;
            const char *t = n->text;
            int first_w = font_stack((unsigned char)t[0])->w;
            int last_w  = font_stack((unsigned char)t[strlen(t) - 1])->w;
            if (last_w >= first_w) round_up = 1;
            n_off = round_up ? (e->w - n->w + 1) / 2 : (e->w - n->w) / 2;
        } else {
            n_off = (e->w - n->w) / 2;
        }
        if (n_off < 0) n_off = 0;
        int n_x = x + n_off;
        int d_adv = d->w + trailing_pad(d);
        int d_off = (e->w - d_adv - 4) / 2;
        if (d_off < 1) d_off = 1;
        int d_x = x + d_off;
        place_and_draw(ctx, bm, n, n_x, y);
        bm_hline(bm, x, y + n->h + 1, e->w, 1);
        place_and_draw(ctx, bm, d, d_x, y + n->h + 3);
        break;
    }

    case EX_POW: {
        expr *base = e->kids[0], *ex = e->kids[1];
        int raise = ex->h - 1;
        int base_y = y + raise;
        int cx = x;
        int wpb = needs_parens(EX_POW, 0, base->kind);
        int last_base_w = wpb ? 5 : last_glyph_w(base);
        int first_ex_w  = first_glyph_w(ex);
        int gap = font_hp_advance(last_base_w, first_ex_w) - last_base_w;
        if (gap < 1) gap = 1;
        if (wpb) {
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
        cx += gap;
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
        int vert_x = x + hw - 1;        /* vertical hook column */
        int sqrt_h = body->h + 3;
        glyph_draw_sqrt_hook(bm, x, y, sqrt_h, 1);
        /* Vinculum from vertical to right edge */
        bm_hline(bm, vert_x, y, e->w - hw + 1, 1);
        /* Right cap: 2 rows below vinculum at rightmost col */
        int right_x = x + e->w - 1;
        bm_set(bm, right_x, y + 1, 1);
        bm_set(bm, right_x, y + 2, 1);
        /* Body offset: 2 cols right of vertical, 2 rows below vinculum */
        place_and_draw(ctx, bm, body, vert_x + 2, y + 2);
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
        int sigma_h = body->h + 6;
        if (sigma_h < 13) sigma_h = 13;
        int sigma_w = sigma_h;
        int atomic_lo = (var->kind == EX_NUM || var->kind == EX_NAME) &&
                        (lo->kind == EX_NUM || lo->kind == EX_NAME);
        int low_w;
        if (atomic_lo) {
            char buf[80];
            snprintf(buf, sizeof(buf), "%s=%s",
                     var->text ? var->text : "",
                     lo->text ? lo->text : "");
            low_w = font_measure_stack(buf);
        } else {
            low_w = var->w + 7 + lo->w;
        }
        int top_w = hi->w;
        int sub_w = low_w > top_w ? low_w : top_w;
        if (sub_w < sigma_w) sub_w = sigma_w;
        /* hi at top, centered horizontally above sigma */
        place_and_draw(ctx, bm, hi, x + (sub_w - hi->w) / 2, y);
        /* sigma in the middle (after hi + 1 gap row) */
        int sigma_y = y + hi->h + 1;
        int sigma_x = x + (sub_w - sigma_w) / 2;
        glyph_draw_sigma(bm, sigma_x, sigma_y, sigma_h, 1);
        /* var=lo at the bottom (after sigma + 1 gap row) */
        int low_x = x + (sub_w - low_w) / 2;
        int low_y = y + hi->h + 1 + sigma_h + 1;
        if (atomic_lo) {
            char buf[80];
            snprintf(buf, sizeof(buf), "%s=%s",
                     var->text ? var->text : "",
                     lo->text ? lo->text : "");
            font_draw_stack(bm, low_x, low_y, buf, 1);
        } else {
            place_and_draw(ctx, bm, var, low_x, low_y);
            const glyph_t *geq = font_stack('=');
            bm_blit_glyph(bm, low_x + var->w + 1,
                          low_y + var->axis - geq->axis,
                          geq->rows, geq->w, geq->h, 1);
            place_and_draw(ctx, bm, lo, low_x + var->w + 7, low_y);
        }
        /* body to right of sigma, vertically centered with sigma */
        place_and_draw(ctx, bm, body, x + sub_w + 3, y + e->axis - body->axis);
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
    /* HP centering: for everything except DIV, reserve 4 cols of cursor
       space on the right plus the trailing pad of the last glyph; DIV gets
       no reserve since the cursor is inside the bar slot.
       Per HP: leftmost = floor((127 - measure_with_last_advance) / 2),
       which is equivalent to (131 - root.w - 4 - trailing_pad) / 2 if
       root.w is computed without including the last glyph's trailing pad. */
    /* When the root's cursor lives INSIDE its own body (DIV, SQRT, parens,
       function args, etc.), no extra cursor reserve is needed at the root.
       For atomic / linear roots (NUM, NAME, ADD/SUB/MUL/EQ, POW, NEG), the
       cursor lands to the right of the root, so reserve a column block. */
    int reserve;
    switch (root->kind) {
    case EX_DIV: case EX_SQRT: case EX_NTHROOT: case EX_PAREN: case EX_ABS:
    case EX_FUNC: case EX_USERFUNC:
    case EX_INTEG: case EX_SUM: case EX_DERIV: case EX_WHERE:
    case EX_CMPLX:
        reserve = 0; break;
    default:
        reserve = 4 + trailing_pad(root); break;
    }
    /* Walk the rightmost binary chain.  If we end at a paren-trailing
       structure, the layout has 3 cols of slack past the visible ')'
       (paren advance 6 vs visible ink 2 + 1 col gap).  Subtract that
       slack when centering and use no extra reserve, matching HP. */
    {
        const expr *rmost = root;
        int saw_paren_wrap = 0;
        while (rmost) {
            const expr *next = NULL;
            int idx = -1;
            switch (rmost->kind) {
            case EX_ADD: case EX_SUB: case EX_MUL: case EX_IMUL: case EX_EQ:
            case EX_POW:
                next = rmost->kids[1]; idx = 1; break;
            case EX_NEG:
                next = rmost->kids[0]; idx = 0; break;
            default: break;
            }
            if (!next) break;
            /* If `next` is wrapped in parens by its parent (rmost), the
               visible right of the equation is the closing paren. */
            if (needs_parens(rmost->kind, idx, next->kind)) saw_paren_wrap = 1;
            rmost = next;
        }
        if (saw_paren_wrap) {
            /* Visual ends with a paren ')' — same offset as paren-trailing. */
            if (root->kind != EX_PAREN && root->kind != EX_ABS) {
                reserve = -3;
            }
        } else if (rmost) {
            switch (rmost->kind) {
            case EX_PAREN: case EX_ABS:
            case EX_FUNC: case EX_USERFUNC:
            case EX_DERIV: case EX_CMPLX:
                if (root->kind != rmost->kind) {
                    reserve = -3;
                }
                break;
            case EX_SQRT: case EX_NTHROOT:
                if (root->kind != rmost->kind) {
                    reserve = -5;
                }
                break;
            default: break;
            }
        }
    }
    int x = (bm->w - root->w - reserve) / 2;
    int y = (bm->h - root->h) / 2;
    if (x < 1) x = 1;
    if (y < 0) y = 0;
    if (x + root->w > bm->w - 1) x = bm->w - 1 - root->w;
    if (x < 0) x = 0;
    place_and_draw(ctx, bm, root, x, y);

    /* Selection box: a 1-px border with 1-px gap around the selected
       sub-expression (per Meta Kernel §8.1.2 / §8.8.1).  HP's actual LCD
       uses inverse-video — invisible after binary thresholding — so we
       skip drawing the box only when rendering for headless snapshots
       (caret_visible == 0). */
    if (ctx->sel && ctx->caret_visible) {
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
