#ifndef SDLEQW_EXPR_H
#define SDLEQW_EXPR_H

typedef enum {
    EX_NUM,        /* literal: text holds digits */
    EX_NAME,       /* identifier: text holds name */
    EX_PLACEHOLDER,/* empty argument slot */

    EX_NEG,        /* unary minus */
    EX_ADD, EX_SUB,
    EX_MUL,        /* explicit * */
    EX_IMUL,       /* implicit multiply (no symbol drawn) */
    EX_DIV,        /* stacked fraction */
    EX_POW,        /* a^b */
    EX_PAREN,      /* explicit parens */
    EX_ABS,        /* |a| */
    EX_SQRT,       /* sqrt(a) */
    EX_NTHROOT,    /* nth-root: kids[0]=index, kids[1]=body */
    EX_FUNC,       /* prefix function: text=name, kids=args */
    EX_USERFUNC,   /* user-defined: text=name, kids=args */
    EX_DERIV,      /* d/dx body: kids[0]=var, kids[1]=body */
    EX_INTEG,      /* integral: kids[0]=lo, [1]=hi, [2]=body, [3]=var */
    EX_SUM,        /* sum: kids[0]=var, [1]=lo, [2]=hi, [3]=body */
    EX_WHERE,      /* body | var=val: kids[0]=body, [1]=var, [2]=val */
    EX_CMPLX,      /* (re,im) complex literal */
    EX_EQ,         /* lhs = rhs */
} expr_kind;

typedef struct expr expr;
struct expr {
    expr_kind kind;
    char     *text;          /* for NUM/NAME/FUNC: name or value */
    expr     *parent;
    expr    **kids;
    int       nkids;
    int       kidcap;

    /* Layout cache, filled by render pass. */
    int x, y;                /* top-left position on bitmap */
    int w, h;                /* size */
    int axis;                /* y of math axis within node (relative to top) */
    int use_mini;            /* whether laid out in mini font */
};

/* Allocation / construction. */
expr *expr_new(expr_kind k);
expr *expr_num(const char *s);
expr *expr_name(const char *s);
expr *expr_placeholder(void);
expr *expr_unary(expr_kind k, expr *a);
expr *expr_binary(expr_kind k, expr *a, expr *b);
expr *expr_with_kids(expr_kind k, int n, expr **kids);
void  expr_free(expr *e);

/* Tree manipulation. */
void  expr_add_kid(expr *p, expr *k);            /* append, sets parent */
void  expr_set_kid(expr *p, int i, expr *k);     /* replace kid i; frees old if not NULL */
void  expr_replace_in_parent(expr *old, expr *neu); /* swap old with neu in old's parent */
int   expr_index_in_parent(const expr *e);       /* -1 if root */
expr *expr_root(expr *e);
int   expr_complete(const expr *e);              /* returns 0 if any placeholder reachable */

/* Text editing for NUM/NAME (and turning PLACEHOLDER into NAME/NUM). */
void  expr_text_set(expr *e, const char *s);
void  expr_text_append(expr *e, char c);
void  expr_text_backspace(expr *e);              /* deletes last char; if empty -> placeholder */

/* Pretty linear printer (for stack representation / debug). Caller frees. */
char *expr_to_string(const expr *e);

#endif
