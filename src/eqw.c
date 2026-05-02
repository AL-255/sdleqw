#include "eqw.h"
#include "font.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/* ---------- helpers ---------- */

static void set_status(eqw_t *e, const char *s)
{
    strncpy(e->status, s ? s : "", sizeof(e->status) - 1);
    e->status[sizeof(e->status) - 1] = 0;
}

/* Replace `old` (anywhere in tree) with `neu`. If old is the root, root is replaced. */
static void replace_node(eqw_t *e, expr *old, expr *neu)
{
    if (old == e->root) {
        e->root = neu;
        if (neu) neu->parent = NULL;
        if (old) { old->parent = NULL; expr_free(old); }
    } else {
        expr_replace_in_parent(old, neu);
    }
}

/* Wrap the current selection in `wrapper`. The current selection ends up at
   wrapper->kids[sel_kid_idx]; the previous slot at that index is freed.
   Then the selection becomes either wrapper itself (if focus_kid<0) or
   wrapper->kids[focus_kid]; if focus_kid points at a placeholder, we enter
   edit mode on it. */
static void wrap_sel(eqw_t *e, expr *wrapper, int sel_kid_idx, int focus_kid)
{
    expr *parent = NULL;
    int   idx    = -1;
    expr *cur    = e->sel;

    if (cur == e->root) {
        parent = NULL;
    } else if (cur && cur->parent) {
        parent = cur->parent;
        idx = expr_index_in_parent(cur);
        cur->parent = NULL;
        parent->kids[idx] = NULL;
    } else {
        /* shouldn't happen */
        return;
    }

    /* Replace wrapper's slot with cur. */
    if (wrapper->kids[sel_kid_idx]) {
        expr_free(wrapper->kids[sel_kid_idx]);
        wrapper->kids[sel_kid_idx] = NULL;
    }
    wrapper->kids[sel_kid_idx] = cur;
    if (cur) cur->parent = wrapper;

    if (parent) {
        parent->kids[idx] = wrapper;
        wrapper->parent = parent;
    } else {
        e->root = wrapper;
        wrapper->parent = NULL;
    }

    if (focus_kid >= 0 && focus_kid < wrapper->nkids) {
        e->sel = wrapper->kids[focus_kid];
        if (e->sel->kind == EX_PLACEHOLDER) {
            e->edit = e->sel;
            e->mode = EQW_MODE_EDIT;
        } else {
            e->edit = NULL;
            e->mode = EQW_MODE_SEL;
        }
    } else {
        e->sel = wrapper;
        e->edit = NULL;
        e->mode = EQW_MODE_SEL;
    }
}

/* Same but the wrapper has no placeholder — it just consumes the selection.
   If the selection was a placeholder, stay in edit mode on it inside the
   wrapper (so a subsequent char fills the inside, not replaces the wrapper). */
static void wrap_sel_just_consume(eqw_t *e, expr_kind k, int focus_back)
{
    expr *cur = e->sel;
    if (!cur) return;
    int was_placeholder = (cur->kind == EX_PLACEHOLDER);
    expr *parent = (cur == e->root) ? NULL : cur->parent;
    int idx = (parent) ? expr_index_in_parent(cur) : -1;
    if (parent) {
        parent->kids[idx] = NULL;
        cur->parent = NULL;
    } else {
        e->root = NULL;
    }
    expr *wrapper = expr_new(k);
    expr_add_kid(wrapper, cur);
    if (parent) {
        parent->kids[idx] = wrapper;
        wrapper->parent = parent;
    } else {
        e->root = wrapper;
        wrapper->parent = NULL;
    }
    if (was_placeholder) {
        e->sel = wrapper->kids[0];
        e->edit = e->sel;
        e->mode = EQW_MODE_EDIT;
    } else {
        e->sel = focus_back ? wrapper->kids[0] : wrapper;
        e->edit = NULL;
        e->mode = EQW_MODE_SEL;
    }
}

/* ---------- character entry ---------- */

/* Forward decl for auto-multiply. */
static void op_binary(eqw_t *e, expr_kind k);

/* True iff we're editing an atomic NUM or NAME token. */
static int editing_atomic(const eqw_t *e)
{
    return e->mode == EQW_MODE_EDIT && e->edit &&
           (e->edit->kind == EX_NUM || e->edit->kind == EX_NAME);
}

/* Per Meta Kernel doc 8.2.4: a NUM followed by an alpha character (or a
   prefix function, or a sqrt, etc.) auto-inserts an implicit multiply.
   Similarly a NAME followed by a prefix-fn auto-multiplies. After this
   returns 1, we're in edit mode on a fresh placeholder. */
static int auto_imul_for_kind(eqw_t *e, int trigger)
{
    /* trigger == 'A' for an alpha char, 'F' for a prefix-fn (incl. sqrt). */
    if (e->mode != EQW_MODE_EDIT || !e->edit) return 0;
    if (trigger == 'A' && e->edit->kind == EX_NUM) {
        op_binary(e, EX_MUL);
        return 1;
    }
    if (trigger == 'F' && (e->edit->kind == EX_NUM || e->edit->kind == EX_NAME)) {
        op_binary(e, EX_MUL);
        return 1;
    }
    return 0;
}

static void apply_char(eqw_t *e, int c)
{
    /* HP alpha mode produces uppercase letters by default. To match that
       visually (since our test generator sends lowercase too), normalize
       letters to uppercase here. */
    if (isalpha(c)) c = toupper(c);
    /* Auto-multiply: editing a NUM, alpha char arrives -> commit and split. */
    if (e->mode == EQW_MODE_EDIT && e->edit && e->edit->kind == EX_NUM &&
        isalpha(c)) {
        auto_imul_for_kind(e, 'A');
        /* now in EDIT on a fresh placeholder; fall through to insert. */
    }
    /* In selection mode: replace selection with a fresh NUM/NAME containing c.
       In edit mode: append c to the current edit node's text. If the edit
       node is a placeholder, convert it. */
    if (e->mode != EQW_MODE_EDIT) {
        if (!e->sel) return;
        expr *fresh = NULL;
        if (isdigit(c) || c == '.') {
            fresh = expr_new(EX_NUM);
            char b[2] = { (char)c, 0 };
            expr_text_set(fresh, b);
        } else {
            fresh = expr_new(EX_NAME);
            char b[2] = { (char)c, 0 };
            expr_text_set(fresh, b);
        }
        if (e->sel->kind == EX_PLACEHOLDER || e->sel->kind == EX_NUM || e->sel->kind == EX_NAME) {
            replace_node(e, e->sel, fresh);
            e->sel = fresh;
        } else {
            /* Replace whole subtree (8.3.5: typing chars suppresses selection). */
            replace_node(e, e->sel, fresh);
            e->sel = fresh;
        }
        e->edit = fresh;
        e->mode = EQW_MODE_EDIT;
        return;
    }
    /* edit mode */
    if (!e->edit) return;
    if (e->edit->kind == EX_PLACEHOLDER) {
        e->edit->kind = (isdigit(c) || c == '.') ? EX_NUM : EX_NAME;
        char b[2] = { (char)c, 0 };
        expr_text_set(e->edit, b);
    } else {
        expr_text_append(e->edit, (char)c);
    }
}

/* Commit current edit (no-op other than mode change). */
static void commit_edit(eqw_t *e)
{
    if (e->mode == EQW_MODE_EDIT) {
        e->edit = NULL;
        e->mode = EQW_MODE_SEL;
    }
}

/* Implicit-multiply auto-insertion when typing alpha char while editing a
   number, per 8.2.4. We approximate this for simplicity by *not* doing it,
   because we don't track an "edited expression" beyond the current text node.
   A real EQW would split the number. We leave it for now. */

/* ---------- ops ---------- */

static void op_binary(eqw_t *e, expr_kind k)
{
    /* In edit mode: commit, then act in selection mode. */
    if (e->mode == EQW_MODE_EDIT) commit_edit(e);
    if (!e->sel) return;
    expr *wrap = expr_new(k);
    expr_add_kid(wrap, expr_placeholder()); /* slot 0 placeholder; will be replaced by sel */
    expr_add_kid(wrap, expr_placeholder()); /* slot 1 placeholder */
    wrap_sel(e, wrap, 0, 1);
}

static void op_pow(eqw_t *e)            { op_binary(e, EX_POW); }
static void op_div(eqw_t *e)            { op_binary(e, EX_DIV); }
static void op_add(eqw_t *e)            { op_binary(e, EX_ADD); }
static void op_sub(eqw_t *e)
{
    /* HP behavior: pressing - on an empty placeholder creates unary NEG, not
       binary SUB. Detect by checking if the current selection is a fresh
       placeholder. */
    if (e->sel && e->sel->kind == EX_PLACEHOLDER) {
        /* Wrap placeholder in NEG containing a placeholder body. */
        expr *cur = e->sel;
        expr *parent = (cur == e->root) ? NULL : cur->parent;
        int idx = parent ? expr_index_in_parent(cur) : -1;
        if (parent) {
            parent->kids[idx] = NULL;
            cur->parent = NULL;
        } else {
            e->root = NULL;
        }
        expr *neg = expr_new(EX_NEG);
        expr_add_kid(neg, cur);
        if (parent) {
            parent->kids[idx] = neg;
            neg->parent = parent;
        } else {
            e->root = neg;
            neg->parent = NULL;
        }
        e->sel = cur;     /* the placeholder inside NEG */
        e->edit = cur;
        e->mode = EQW_MODE_EDIT;
        return;
    }
    op_binary(e, EX_SUB);
}
static void op_mul(eqw_t *e)            { op_binary(e, EX_MUL); }

static void op_sqrt(eqw_t *e)
{
    if (e->mode == EQW_MODE_EDIT) commit_edit(e);
    if (!e->sel) return;
    wrap_sel_just_consume(e, EX_SQRT, 0);
}

static void op_nthroot(eqw_t *e)
{
    if (e->mode == EQW_MODE_EDIT) commit_edit(e);
    if (!e->sel) return;
    expr *wrap = expr_new(EX_NTHROOT);
    expr_add_kid(wrap, expr_placeholder()); /* index */
    expr_add_kid(wrap, expr_placeholder()); /* body */
    wrap_sel(e, wrap, 1, 0);
}

static void op_paren(eqw_t *e)
{
    if (e->mode == EQW_MODE_EDIT) commit_edit(e);
    if (!e->sel) return;
    wrap_sel_just_consume(e, EX_PAREN, 0);
}

static void op_abs(eqw_t *e)
{
    if (e->mode == EQW_MODE_EDIT) commit_edit(e);
    if (!e->sel) return;
    wrap_sel_just_consume(e, EX_ABS, 0);
}

static void op_integ(eqw_t *e)
{
    if (e->mode == EQW_MODE_EDIT) commit_edit(e);
    if (!e->sel) return;
    expr *wrap = expr_new(EX_INTEG);
    expr_add_kid(wrap, expr_placeholder()); /* lo */
    expr_add_kid(wrap, expr_placeholder()); /* hi */
    expr_add_kid(wrap, expr_placeholder()); /* body */
    expr_add_kid(wrap, expr_placeholder()); /* var */
    wrap_sel(e, wrap, 2, 0); /* selection becomes body, then jump to lo for editing */
}

static void op_deriv(eqw_t *e)
{
    if (e->mode == EQW_MODE_EDIT) commit_edit(e);
    if (!e->sel) return;
    expr *wrap = expr_new(EX_DERIV);
    expr_add_kid(wrap, expr_placeholder()); /* var */
    expr_add_kid(wrap, expr_placeholder()); /* body */
    wrap_sel(e, wrap, 1, 0);
}

static void op_sum(eqw_t *e)
{
    if (e->mode == EQW_MODE_EDIT) commit_edit(e);
    if (!e->sel) return;
    expr *wrap = expr_new(EX_SUM);
    expr_add_kid(wrap, expr_placeholder()); /* var */
    expr_add_kid(wrap, expr_placeholder()); /* lo */
    expr_add_kid(wrap, expr_placeholder()); /* hi */
    expr_add_kid(wrap, expr_placeholder()); /* body */
    wrap_sel(e, wrap, 3, 0);
}

static void op_where(eqw_t *e)
{
    if (e->mode == EQW_MODE_EDIT) commit_edit(e);
    if (!e->sel) return;
    expr *wrap = expr_new(EX_WHERE);
    expr_add_kid(wrap, expr_placeholder()); /* body */
    expr_add_kid(wrap, expr_placeholder()); /* var */
    expr_add_kid(wrap, expr_placeholder()); /* val */
    wrap_sel(e, wrap, 0, 1);
}

static void op_cmplx(eqw_t *e)
{
    if (e->mode == EQW_MODE_EDIT) commit_edit(e);
    if (!e->sel) return;
    expr *wrap = expr_new(EX_CMPLX);
    expr_add_kid(wrap, expr_placeholder()); /* re */
    expr_add_kid(wrap, expr_placeholder()); /* im */
    wrap_sel(e, wrap, 0, 1);
}

static void op_func_apply(eqw_t *e, const char *name)
{
    /* Per Meta Kernel doc 8.2.4: NUM/NAME followed by a prefix function
       triggers auto-implicit-multiply. The "7 SIN" example produces 7·SIN(?). */
    int was_atom = (e->mode == EQW_MODE_EDIT && e->edit &&
                    (e->edit->kind == EX_NUM || e->edit->kind == EX_NAME));
    if (was_atom) {
        op_binary(e, EX_MUL);
        /* Now sel = placeholder, edit = placeholder, mode = EDIT, after the
           NUM/NAME we just committed. Fall through to put a FUNC there. */
    }
    if (e->mode == EQW_MODE_EDIT) commit_edit(e);
    if (!e->sel) return;
    expr *wrap = expr_new(EX_FUNC);
    expr_text_set(wrap, name);
    expr_add_kid(wrap, expr_placeholder());
    if (e->sel->kind == EX_PLACEHOLDER) {
        /* Replace the placeholder with the FUNC and focus on its argument. */
        replace_node(e, e->sel, wrap);
        e->sel = wrap->kids[0];
        e->edit = e->sel;
        e->mode = EQW_MODE_EDIT;
    } else {
        wrap_sel(e, wrap, 0, -1);
    }
}

static void op_userfunc(eqw_t *e)
{
    /* Convert current text edit into a USERFUNC name with first arg placeholder. */
    if (e->mode == EQW_MODE_EDIT && e->edit && e->edit->kind == EX_NAME) {
        /* Promote NAME to USERFUNC keeping the text as name. */
        expr *node = e->edit;
        expr *wrap = expr_new(EX_USERFUNC);
        expr_text_set(wrap, node->text ? node->text : "F");
        expr_add_kid(wrap, expr_placeholder());
        replace_node(e, node, wrap);
        e->sel = wrap->kids[0];
        e->edit = e->sel;
        e->mode = EQW_MODE_EDIT;
        return;
    }
}

/* Add another argument to USERFUNC under the selection. */
static void op_next_arg(eqw_t *e)
{
    if (e->mode == EQW_MODE_EDIT) commit_edit(e);
    expr *cur = e->sel;
    if (!cur) return;
    /* Walk up to find a multi-arg parent; we want the next sibling. */
    expr *p = cur->parent;
    if (!p) return;
    int idx = expr_index_in_parent(cur);
    if (idx + 1 < p->nkids) {
        e->sel = p->kids[idx + 1];
        if (e->sel->kind == EX_PLACEHOLDER) {
            e->edit = e->sel;
            e->mode = EQW_MODE_EDIT;
        } else {
            e->edit = NULL;
            e->mode = EQW_MODE_SEL;
        }
        return;
    }
    /* If parent is a USERFUNC, append new placeholder. */
    if (p->kind == EX_USERFUNC) {
        expr *ph = expr_placeholder();
        expr_add_kid(p, ph);
        e->sel = ph;
        e->edit = ph;
        e->mode = EQW_MODE_EDIT;
        return;
    }
    /* Otherwise climb up. */
    while (p) {
        int pi = expr_index_in_parent(p);
        if (p->parent && pi + 1 < p->parent->nkids) {
            e->sel = p->parent->kids[pi + 1];
            if (e->sel->kind == EX_PLACEHOLDER) {
                e->edit = e->sel;
                e->mode = EQW_MODE_EDIT;
            } else {
                e->edit = NULL;
                e->mode = EQW_MODE_SEL;
            }
            return;
        }
        p = p->parent;
    }
}

/* ---------- navigation in selection mode ---------- */

static void nav_prev_sibling(eqw_t *e)
{
    if (!e->sel || !e->sel->parent) return;
    int idx = expr_index_in_parent(e->sel);
    if (idx > 0) e->sel = e->sel->parent->kids[idx - 1];
}

static void nav_next_sibling(eqw_t *e)
{
    if (!e->sel || !e->sel->parent) return;
    int idx = expr_index_in_parent(e->sel);
    if (idx + 1 < e->sel->parent->nkids) e->sel = e->sel->parent->kids[idx + 1];
    else if (e->sel->parent && e->sel->parent->parent) e->sel = e->sel->parent;
}

static void nav_up(eqw_t *e)
{
    if (e->sel && e->sel->parent) e->sel = e->sel->parent;
}

static void nav_down(eqw_t *e)
{
    if (e->sel && e->sel->nkids > 0) e->sel = e->sel->kids[0];
}

/* ---------- deletions ---------- */

static void del_one_arg_func(eqw_t *e)
{
    expr *cur = e->sel;
    if (!cur || cur->nkids != 1) {
        set_status(e, "1 Arg Function Needed");
        return;
    }
    expr *only = cur->kids[0];
    cur->kids[0] = NULL;
    only->parent = NULL;
    replace_node(e, cur, only);
    e->sel = only;
}

static void del_arg_of_two(eqw_t *e)
{
    expr *cur = e->sel;
    if (!cur || !cur->parent || cur->parent->nkids < 2) {
        set_status(e, "Two Arguments Expected");
        return;
    }
    expr *p = cur->parent;
    int idx = expr_index_in_parent(cur);
    int keep = idx == 0 ? 1 : 0;
    expr *kept = p->kids[keep];
    p->kids[keep] = NULL;
    kept->parent = NULL;
    replace_node(e, p, kept);
    e->sel = kept;
}

static void del_select_only(eqw_t *e)
{
    /* Delete all but the selection: replace root with sel. */
    expr *cur = e->sel;
    if (!cur || cur == e->root) {
        set_status(e, "Select an Argument");
        return;
    }
    expr *p = cur->parent;
    int idx = expr_index_in_parent(cur);
    p->kids[idx] = NULL;
    cur->parent = NULL;
    expr_free(e->root);
    e->root = cur;
    e->sel = cur;
}

/* ---------- public ---------- */

eqw_t *eqw_new(void)
{
    eqw_t *e = calloc(1, sizeof(*e));
    e->root = expr_placeholder();
    e->sel = e->root;
    e->mode = EQW_MODE_EDIT;
    e->edit = e->root;
    e->use_mini = 0;
    set_status(e, "");
    return e;
}

void eqw_free(eqw_t *e)
{
    if (!e) return;
    expr_free(e->root);
    free(e);
}

void eqw_load(eqw_t *e, expr *root)
{
    if (!root) return;
    expr_free(e->root);
    e->root = root;
    e->sel = root;
    e->edit = NULL;
    e->mode = EQW_MODE_SEL;
}

/* Demo 0: E = ∫_0^{1/X} |F(t)| dt (Meta Kernel doc, section 8 example) */
static expr *demo_integral(void)
{
    expr *func_F = expr_with_kids(EX_FUNC, 1, (expr*[]){ expr_name("t") });
    expr_text_set(func_F, "F");
    expr *body = expr_unary(EX_ABS, func_F);
    expr *integ = expr_with_kids(EX_INTEG, 4, (expr*[]){
        expr_num("0"),
        expr_binary(EX_DIV, expr_num("1"), expr_name("X")),
        body,
        expr_name("t")
    });
    return expr_binary(EX_EQ, expr_name("E"), integ);
}

/* Demo 1: ∂x ( cos(x) / sin(x)^2 )  (Meta Kernel doc) */
static expr *demo_deriv(void)
{
    expr *cos_x = expr_with_kids(EX_FUNC, 1, (expr*[]){ expr_name("x") });
    expr_text_set(cos_x, "COS");
    expr *sin_x = expr_with_kids(EX_FUNC, 1, (expr*[]){ expr_name("x") });
    expr_text_set(sin_x, "SIN");
    expr *sin_sq = expr_binary(EX_POW, sin_x, expr_num("2"));
    expr *frac   = expr_binary(EX_DIV, cos_x, sin_sq);
    return expr_binary(EX_DERIV, expr_name("x"), frac);
}

/* Demo 2: 5*(1+1/7.5)/(sqrt(3) - 2^3)   (HP user guide page 2-1) */
static expr *demo_arith(void)
{
    expr *inner = expr_binary(EX_ADD,
                              expr_num("1"),
                              expr_binary(EX_DIV, expr_num("1"), expr_num("7.5")));
    expr *num = expr_binary(EX_MUL, expr_num("5"), inner);
    expr *den = expr_binary(EX_SUB,
                            expr_unary(EX_SQRT, expr_num("3")),
                            expr_binary(EX_POW, expr_num("2"), expr_num("3")));
    return expr_binary(EX_DIV, num, den);
}

/* Demo 3: 2*L*sqrt(1 + x/R) / (R+y) + 2*L/b   (HP user guide) */
static expr *demo_algebra(void)
{
    expr *inner = expr_binary(EX_ADD, expr_num("1"),
                              expr_binary(EX_DIV, expr_name("x"), expr_name("R")));
    expr *sq = expr_unary(EX_SQRT, inner);
    expr *num = expr_binary(EX_MUL, expr_binary(EX_MUL, expr_num("2"), expr_name("L")), sq);
    expr *den = expr_binary(EX_ADD, expr_name("R"), expr_name("y"));
    expr *frac = expr_binary(EX_DIV, num, den);
    expr *tail = expr_binary(EX_DIV,
                             expr_binary(EX_MUL, expr_num("2"), expr_name("L")),
                             expr_name("b"));
    return expr_binary(EX_ADD, frac, tail);
}

/* Demo 4: Σ_{k=1}^{n} k^2 */
static expr *demo_sum(void)
{
    expr *body = expr_binary(EX_POW, expr_name("k"), expr_num("2"));
    return expr_with_kids(EX_SUM, 4, (expr*[]){
        expr_name("k"), expr_num("1"), expr_name("n"), body
    });
}

typedef expr *(*demo_fn)(void);
static const demo_fn DEMOS[] = {
    demo_integral, demo_deriv, demo_arith, demo_algebra, demo_sum
};

int   eqw_demo_count(void) { return (int)(sizeof(DEMOS)/sizeof(DEMOS[0])); }
expr *eqw_demo_expr_n(int which)
{
    int n = eqw_demo_count();
    if (which < 0) which = 0;
    if (which >= n) which = n - 1;
    return DEMOS[which]();
}
expr *eqw_demo_expr(void) { return demo_integral(); }

const char *eqw_mode_name(const eqw_t *e)
{
    switch (e->mode) {
    case EQW_MODE_SEL: return "SEL";
    case EQW_MODE_EDIT: return "EDIT";
    case EQW_MODE_CURSOR: return "CURS";
    }
    return "";
}

/* ---------- key dispatch ---------- */

void eqw_handle(eqw_t *e, const eqw_key *k)
{
    if (!k) return;
    /* Cursor mode: only some keys are meaningful. */
    if (e->mode == EQW_MODE_CURSOR) {
        int step = k->shift ? 6 : 2;
        switch (k->k) {
        case EQW_K_LEFT:  e->cursor_x -= step; break;
        case EQW_K_RIGHT: e->cursor_x += step; break;
        case EQW_K_UP:    e->cursor_y -= step; break;
        case EQW_K_DOWN:  e->cursor_y += step; break;
        case EQW_K_ENTER: {
            expr *hit = render_hit(e->root, e->cursor_x, e->cursor_y);
            if (hit) e->sel = hit;
            e->mode = EQW_MODE_SEL;
            break;
        }
        case EQW_K_CURSOR:
        case EQW_K_TAB:   e->mode = EQW_MODE_SEL; break;
        case EQW_K_ESC:   e->exit_code = 2; break;
        default: break;
        }
        if (e->cursor_x < 0) e->cursor_x = 0;
        if (e->cursor_y < 0) e->cursor_y = 0;
        return;
    }

    set_status(e, "");
    switch (k->k) {
    case EQW_K_NONE: break;
    case EQW_K_CHAR:
        apply_char(e, k->ch);
        break;
    case EQW_K_PLUS:    op_add(e); break;
    case EQW_K_MINUS:   op_sub(e); break;
    case EQW_K_MUL:     op_mul(e); break;
    case EQW_K_DIV:     op_div(e); break;
    case EQW_K_POW:     op_pow(e); break;
    case EQW_K_SQRT:    op_sqrt(e); break;
    case EQW_K_NTHROOT: op_nthroot(e); break;
    case EQW_K_PAREN:   op_paren(e); break;
    case EQW_K_ABS:     op_abs(e); break;
    case EQW_K_INTEG:   op_integ(e); break;
    case EQW_K_DERIV:   op_deriv(e); break;
    case EQW_K_SUM:     op_sum(e); break;
    case EQW_K_WHERE:   op_where(e); break;
    case EQW_K_CMPLX:   op_cmplx(e); break;
    case EQW_K_USERFUNC:op_userfunc(e); break;
    case EQW_K_FUNC_SIN: op_func_apply(e, "SIN"); break;
    case EQW_K_FUNC_COS: op_func_apply(e, "COS"); break;
    case EQW_K_FUNC_TAN: op_func_apply(e, "TAN"); break;
    case EQW_K_FUNC_LN:  op_func_apply(e, "LN"); break;
    case EQW_K_FUNC_EXP: op_func_apply(e, "EXP"); break;
    case EQW_K_FUNC_LOG: op_func_apply(e, "LOG"); break;
    case EQW_K_FUNC_ABS: op_func_apply(e, "ABS"); break;
    case EQW_K_COMMA:   op_next_arg(e); break;
    case EQW_K_LEFT:
        if (e->mode == EQW_MODE_EDIT) commit_edit(e);
        nav_prev_sibling(e);
        break;
    case EQW_K_RIGHT:
        if (e->mode == EQW_MODE_EDIT) commit_edit(e);
        nav_next_sibling(e);
        break;
    case EQW_K_UP:
        if (e->mode == EQW_MODE_EDIT) commit_edit(e);
        nav_up(e);
        break;
    case EQW_K_DOWN:
        if (e->mode == EQW_MODE_EDIT) commit_edit(e);
        nav_down(e);
        break;
    case EQW_K_TAB:
        if (e->mode == EQW_MODE_EDIT) commit_edit(e);
        else if (e->sel && (e->sel->kind == EX_NUM || e->sel->kind == EX_NAME ||
                            e->sel->kind == EX_PLACEHOLDER)) {
            e->edit = e->sel;
            e->mode = EQW_MODE_EDIT;
        }
        break;
    case EQW_K_ENTER:
        if (e->mode == EQW_MODE_EDIT) commit_edit(e);
        if (expr_complete(e->root)) {
            e->exit_code = 1;
        } else {
            set_status(e, "Incomplete Expression");
        }
        break;
    case EQW_K_ESC:
        e->exit_code = 2;
        break;
    case EQW_K_BACKSPACE:
        if (e->mode == EQW_MODE_EDIT && e->edit) {
            expr_text_backspace(e->edit);
            if (e->edit->kind == EX_PLACEHOLDER) {
                /* stay in edit on the placeholder */
            }
        } else if (e->sel) {
            /* selection: replace selection with placeholder, enter edit on it */
            expr *ph = expr_placeholder();
            replace_node(e, e->sel, ph);
            e->sel = ph;
            e->edit = ph;
            e->mode = EQW_MODE_EDIT;
        }
        break;
    case EQW_K_DEL:
        if (e->mode == EQW_MODE_EDIT && e->edit) {
            expr_text_backspace(e->edit);
        } else {
            del_one_arg_func(e);
        }
        break;
    case EQW_K_CLEAR:
        if (e->mode == EQW_MODE_EDIT) commit_edit(e);
        if (k->shift) del_select_only(e);
        else del_arg_of_two(e);
        break;
    case EQW_K_ZOOM:
        e->use_mini = !e->use_mini;
        break;
    case EQW_K_CURSOR:
        if (e->mode == EQW_MODE_EDIT) commit_edit(e);
        e->mode = EQW_MODE_CURSOR;
        if (e->sel) {
            e->cursor_x = e->sel->x + e->sel->w / 2;
            e->cursor_y = e->sel->y + e->sel->h / 2;
        } else {
            e->cursor_x = 60;
            e->cursor_y = 30;
        }
        break;
    case EQW_K_DEMO:
        eqw_load(e, eqw_demo_expr());
        break;
    }
}

/* ---------- rendering ---------- */

static void draw_frame(bitmap_t *bm)
{
    /* Optional 1-px frame around the LCD area. We skip — the LCD itself is
       the bitmap. */
    (void)bm;
}

#define MENU_H 7

/* Pixel-perfect HP menu bitmap (131 cols × 7 rows) extracted from x50ng. */
extern const uint8_t hp_menu_bits[7][17];

static void draw_soft_menu(bitmap_t *bm)
{
    int y0 = bm->h - MENU_H;
    for (int y = 0; y < MENU_H; y++) {
        for (int x = 0; x < 131 && x < bm->w; x++) {
            int byte = x / 8;
            int bit = x % 8;
            int v = (hp_menu_bits[y][byte] >> bit) & 1;
            bm_set(bm, x, y0 + y, v);
        }
    }
}

static void draw_status_strip(eqw_t *e, bitmap_t *bm)
{
    /* Top row, left: short error/status text (only when set). */
    if (e->status[0]) {
        font_draw_stack(bm, 1, 0, e->status, 1);
    }
}

void eqw_render(eqw_t *e, bitmap_t *bm, int caret_visible)
{
    bm_clear(bm);
    draw_frame(bm);

    render_ctx ctx = { 0 };
    ctx.use_mini = e->use_mini;
    ctx.sel = (e->mode == EQW_MODE_SEL) ? e->sel : NULL;
    ctx.sel_boxed = 1;
    ctx.edit = (e->mode == EQW_MODE_EDIT) ? e->edit : NULL;
    ctx.caret_visible = caret_visible;
    ctx.cursor_mode = (e->mode == EQW_MODE_CURSOR);
    ctx.cursor_x = e->cursor_x;
    ctx.cursor_y = e->cursor_y;

    /* Render in the area ABOVE the menu strip. */
    bitmap_t upper = *bm;
    upper.h = bm->h - MENU_H - 1;  /* = 72 for 80-row LCD */
    upper.w = bm->w;               /* full LCD width; centering inside */
    render_eqw(&ctx, &upper, e->root);
    draw_status_strip(e, bm);
    draw_soft_menu(bm);
}
