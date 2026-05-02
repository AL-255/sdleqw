#include "expr.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

static char *xstrdup(const char *s)
{
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char *p = malloc(n);
    memcpy(p, s, n);
    return p;
}

expr *expr_new(expr_kind k)
{
    expr *e = calloc(1, sizeof(*e));
    e->kind = k;
    return e;
}

expr *expr_num(const char *s)
{
    expr *e = expr_new(EX_NUM);
    e->text = xstrdup(s ? s : "0");
    return e;
}

expr *expr_name(const char *s)
{
    expr *e = expr_new(EX_NAME);
    e->text = xstrdup(s ? s : "X");
    return e;
}

expr *expr_placeholder(void)
{
    return expr_new(EX_PLACEHOLDER);
}

expr *expr_unary(expr_kind k, expr *a)
{
    expr *e = expr_new(k);
    expr_add_kid(e, a);
    return e;
}

expr *expr_binary(expr_kind k, expr *a, expr *b)
{
    expr *e = expr_new(k);
    expr_add_kid(e, a);
    expr_add_kid(e, b);
    return e;
}

expr *expr_with_kids(expr_kind k, int n, expr **kids)
{
    expr *e = expr_new(k);
    for (int i = 0; i < n; i++) expr_add_kid(e, kids[i]);
    return e;
}

void expr_free(expr *e)
{
    if (!e) return;
    for (int i = 0; i < e->nkids; i++) expr_free(e->kids[i]);
    free(e->kids);
    free(e->text);
    free(e);
}

void expr_add_kid(expr *p, expr *k)
{
    if (!k) return;
    if (p->nkids >= p->kidcap) {
        p->kidcap = p->kidcap ? p->kidcap * 2 : 4;
        p->kids = realloc(p->kids, sizeof(expr *) * p->kidcap);
    }
    p->kids[p->nkids++] = k;
    k->parent = p;
}

void expr_set_kid(expr *p, int i, expr *k)
{
    if (i < 0 || i >= p->nkids) return;
    if (p->kids[i] == k) return;
    if (p->kids[i]) expr_free(p->kids[i]);
    p->kids[i] = k;
    if (k) k->parent = p;
}

void expr_replace_in_parent(expr *old, expr *neu)
{
    if (!old || !old->parent || !neu) return;
    expr *p = old->parent;
    for (int i = 0; i < p->nkids; i++) {
        if (p->kids[i] == old) {
            p->kids[i] = neu;
            neu->parent = p;
            old->parent = NULL;
            expr_free(old);
            return;
        }
    }
}

int expr_index_in_parent(const expr *e)
{
    if (!e || !e->parent) return -1;
    for (int i = 0; i < e->parent->nkids; i++)
        if (e->parent->kids[i] == e) return i;
    return -1;
}

expr *expr_root(expr *e)
{
    while (e && e->parent) e = e->parent;
    return e;
}

int expr_complete(const expr *e)
{
    if (!e) return 0;
    if (e->kind == EX_PLACEHOLDER) return 0;
    for (int i = 0; i < e->nkids; i++)
        if (!expr_complete(e->kids[i])) return 0;
    return 1;
}

void expr_text_set(expr *e, const char *s)
{
    free(e->text);
    e->text = xstrdup(s);
}

void expr_text_append(expr *e, char c)
{
    if (!e->text) {
        e->text = malloc(2);
        e->text[0] = c;
        e->text[1] = 0;
    } else {
        size_t n = strlen(e->text);
        e->text = realloc(e->text, n + 2);
        e->text[n] = c;
        e->text[n + 1] = 0;
    }
}

void expr_text_backspace(expr *e)
{
    if (!e->text) return;
    size_t n = strlen(e->text);
    if (n > 0) e->text[n - 1] = 0;
    if (e->text[0] == 0) {
        free(e->text);
        e->text = NULL;
        e->kind = EX_PLACEHOLDER;
    }
}

/* Linear printer (RPN-ish algebraic). */
static void buf_grow(char **b, size_t *cap, size_t need)
{
    if (need <= *cap) return;
    while (*cap < need) *cap = *cap ? *cap * 2 : 64;
    *b = realloc(*b, *cap);
}
static void buf_putc(char **b, size_t *len, size_t *cap, char c)
{
    buf_grow(b, cap, *len + 2);
    (*b)[(*len)++] = c;
    (*b)[*len] = 0;
}
static void buf_puts(char **b, size_t *len, size_t *cap, const char *s)
{
    if (!s) return;
    size_t n = strlen(s);
    buf_grow(b, cap, *len + n + 1);
    memcpy(*b + *len, s, n);
    *len += n;
    (*b)[*len] = 0;
}

static void to_str_rec(const expr *e, char **b, size_t *len, size_t *cap);

static void emit_args(const expr *e, char **b, size_t *l, size_t *c)
{
    buf_putc(b, l, c, '(');
    for (int i = 0; i < e->nkids; i++) {
        if (i) buf_putc(b, l, c, ',');
        to_str_rec(e->kids[i], b, l, c);
    }
    buf_putc(b, l, c, ')');
}

static void to_str_rec(const expr *e, char **b, size_t *l, size_t *c)
{
    if (!e) return;
    switch (e->kind) {
    case EX_NUM:
    case EX_NAME:
        buf_puts(b, l, c, e->text ? e->text : "");
        break;
    case EX_PLACEHOLDER:
        buf_putc(b, l, c, '?');
        break;
    case EX_NEG:
        buf_putc(b, l, c, '-');
        to_str_rec(e->kids[0], b, l, c);
        break;
    case EX_ADD: to_str_rec(e->kids[0], b, l, c); buf_putc(b, l, c, '+'); to_str_rec(e->kids[1], b, l, c); break;
    case EX_SUB: to_str_rec(e->kids[0], b, l, c); buf_putc(b, l, c, '-'); to_str_rec(e->kids[1], b, l, c); break;
    case EX_MUL: to_str_rec(e->kids[0], b, l, c); buf_putc(b, l, c, '*'); to_str_rec(e->kids[1], b, l, c); break;
    case EX_IMUL: to_str_rec(e->kids[0], b, l, c); buf_putc(b, l, c, '*'); to_str_rec(e->kids[1], b, l, c); break;
    case EX_DIV: to_str_rec(e->kids[0], b, l, c); buf_putc(b, l, c, '/'); to_str_rec(e->kids[1], b, l, c); break;
    case EX_POW: to_str_rec(e->kids[0], b, l, c); buf_putc(b, l, c, '^'); to_str_rec(e->kids[1], b, l, c); break;
    case EX_PAREN: buf_putc(b, l, c, '('); to_str_rec(e->kids[0], b, l, c); buf_putc(b, l, c, ')'); break;
    case EX_ABS: buf_puts(b, l, c, "ABS("); to_str_rec(e->kids[0], b, l, c); buf_putc(b, l, c, ')'); break;
    case EX_SQRT: buf_puts(b, l, c, "SQRT("); to_str_rec(e->kids[0], b, l, c); buf_putc(b, l, c, ')'); break;
    case EX_NTHROOT:
        buf_puts(b, l, c, "XROOT(");
        to_str_rec(e->kids[0], b, l, c); buf_putc(b, l, c, ',');
        to_str_rec(e->kids[1], b, l, c); buf_putc(b, l, c, ')');
        break;
    case EX_FUNC:
    case EX_USERFUNC:
        buf_puts(b, l, c, e->text ? e->text : "?");
        emit_args(e, b, l, c);
        break;
    case EX_DERIV:
        buf_puts(b, l, c, "d");
        to_str_rec(e->kids[0], b, l, c);
        buf_puts(b, l, c, "(");
        to_str_rec(e->kids[1], b, l, c);
        buf_puts(b, l, c, ")");
        break;
    case EX_INTEG:
        buf_puts(b, l, c, "INTVX(");
        for (int i = 0; i < e->nkids; i++) {
            if (i) buf_putc(b, l, c, ',');
            to_str_rec(e->kids[i], b, l, c);
        }
        buf_putc(b, l, c, ')');
        break;
    case EX_SUM:
        buf_puts(b, l, c, "SIGMA(");
        for (int i = 0; i < e->nkids; i++) {
            if (i) buf_putc(b, l, c, ',');
            to_str_rec(e->kids[i], b, l, c);
        }
        buf_putc(b, l, c, ')');
        break;
    case EX_WHERE:
        to_str_rec(e->kids[0], b, l, c);
        buf_putc(b, l, c, '|');
        to_str_rec(e->kids[1], b, l, c);
        buf_putc(b, l, c, '=');
        to_str_rec(e->kids[2], b, l, c);
        break;
    case EX_CMPLX:
        buf_putc(b, l, c, '(');
        to_str_rec(e->kids[0], b, l, c);
        buf_putc(b, l, c, ',');
        to_str_rec(e->kids[1], b, l, c);
        buf_putc(b, l, c, ')');
        break;
    case EX_EQ:
        to_str_rec(e->kids[0], b, l, c);
        buf_putc(b, l, c, '=');
        to_str_rec(e->kids[1], b, l, c);
        break;
    }
}

char *expr_to_string(const expr *e)
{
    char *b = NULL;
    size_t len = 0, cap = 0;
    to_str_rec(e, &b, &len, &cap);
    return b ? b : xstrdup("");
}
