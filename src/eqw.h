#ifndef SDLEQW_EQW_H
#define SDLEQW_EQW_H

#include "bitmap.h"
#include "expr.h"
#include "render.h"

typedef enum {
    EQW_MODE_SEL,
    EQW_MODE_EDIT,
    EQW_MODE_CURSOR,
} eqw_mode;

/* Logical key codes. The frontend translates SDL keys into these. */
typedef enum {
    EQW_K_NONE = 0,
    EQW_K_LEFT, EQW_K_RIGHT, EQW_K_UP, EQW_K_DOWN,
    EQW_K_ENTER, EQW_K_ESC, EQW_K_TAB,
    EQW_K_BACKSPACE,        /* line-edit backspace / "select all but" in selection */
    EQW_K_DEL,              /* in selection: delete 1-arg fn; in edit: forward-delete */
    EQW_K_CLEAR,            /* shift-DEL: delete arg of 2-arg fn */
    EQW_K_PLUS, EQW_K_MINUS, EQW_K_MUL, EQW_K_DIV, EQW_K_POW,
    EQW_K_PAREN, EQW_K_ABS, EQW_K_COMMA,
    EQW_K_SQRT, EQW_K_NTHROOT,
    EQW_K_INTEG, EQW_K_DERIV, EQW_K_SUM, EQW_K_WHERE,
    EQW_K_CMPLX, EQW_K_USERFUNC,
    EQW_K_FUNC_SIN, EQW_K_FUNC_COS, EQW_K_FUNC_TAN,
    EQW_K_FUNC_LN, EQW_K_FUNC_EXP, EQW_K_FUNC_LOG, EQW_K_FUNC_ABS,
    EQW_K_ZOOM,
    EQW_K_CURSOR,
    EQW_K_DEMO,
    EQW_K_CHAR,             /* generic char insertion (digit/letter/symbol) */
} eqw_keycode;

typedef struct {
    eqw_keycode k;
    int  ch;
    int  shift;
} eqw_key;

typedef struct {
    expr     *root;
    expr     *sel;
    expr     *edit;
    eqw_mode  mode;
    int       use_mini;
    int       cursor_x, cursor_y;
    int       cursor_node_x, cursor_node_y; /* used to draw box around hit node */
    expr     *cursor_hit;
    /* a small status string drawn at the bottom row */
    char      status[40];
    /* exit signal: 1 = ENTER (commit), 2 = ESC (cancel), 0 = still running */
    int       exit_code;
} eqw_t;

eqw_t *eqw_new(void);
void   eqw_free(eqw_t *e);

void   eqw_load(eqw_t *e, expr *root);
expr  *eqw_demo_expr(void);
expr  *eqw_demo_expr_n(int which);  /* 0..N-1 */
int    eqw_demo_count(void);

void   eqw_handle(eqw_t *e, const eqw_key *k);
void   eqw_render(eqw_t *e, bitmap_t *bm, int caret_visible);

const char *eqw_mode_name(const eqw_t *e);

#endif
