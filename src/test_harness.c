/* sdleqw behavioral test harness.
 *
 * Each test is a record:
 *   - a starting expression (or NULL = blank in edit mode)
 *   - a key sequence (mini-language)
 *   - the expected linear form of the resulting tree
 *
 * The mini-language for keys:
 *   digit, letter, '.': character (EQW_K_CHAR with that c)
 *   '+' '-' '*' '/' '^': binary ops
 *   '@'  square root
 *   '#'  nth-root
 *   '('  parens wrap
 *   ')'  N/A (the wrap closes itself)
 *   '|'  abs wrap
 *   'I'  integral
 *   'S'  sum
 *   'D'  derivative
 *   'W'  where
 *   'C'  complex
 *   'U'  user-function (promote NAME -> USERFUNC)
 *   ','  next arg (SPC)
 *   '<' '>' 'v' '^' arrows (left, right, down, up).
 *      But '^' is taken for power; we'll use distinct lowercase 'l','r','u','d' for arrows.
 *   We use a distinct escape: '$' followed by one char.
 *     $l left, $r right, $u up, $w down (w as in "wдn"? choose 'k' instead)
 *     $e enter, $x esc, $b backspace, $t tab,
 *     $D del-1arg-fn, $K clear-arg, $L clear-all-but
 *     $z zoom toggle
 *   Function applies (one letter):
 *     $s SIN, $c COS, $T TAN, $n LN, $X EXP, $g LOG, $a ABS
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "expr.h"
#include "eqw.h"

typedef struct {
    const char *name;
    const char *keys;
    const char *expected;
    const char *category;
    expr      *(*starter)(void);  /* NULL = blank */
} testcase_t;

static void send(eqw_t *e, eqw_keycode k, int ch)
{
    eqw_key key = { 0 };
    key.k = k;
    key.ch = ch;
    eqw_handle(e, &key);
}

static int run_seq(eqw_t *e, const char *s)
{
    while (*s) {
        char c = *s++;
        if (c == '$') {
            char esc = *s++;
            switch (esc) {
            case 'l': send(e, EQW_K_LEFT, 0); break;
            case 'r': send(e, EQW_K_RIGHT, 0); break;
            case 'u': send(e, EQW_K_UP, 0); break;
            case 'k': send(e, EQW_K_DOWN, 0); break;
            case 'e': send(e, EQW_K_ENTER, 0); break;
            case 'x': send(e, EQW_K_ESC, 0); break;
            case 'b': send(e, EQW_K_BACKSPACE, 0); break;
            case 't': send(e, EQW_K_TAB, 0); break;
            case 'D': send(e, EQW_K_DEL, 0); break;
            case 'K': send(e, EQW_K_CLEAR, 0); break;
            case 'L': { eqw_key K = {0}; K.k = EQW_K_CLEAR; K.shift = 1; eqw_handle(e, &K); break; }
            case 'z': send(e, EQW_K_ZOOM, 0); break;
            case 's': send(e, EQW_K_FUNC_SIN, 0); break;
            case 'c': send(e, EQW_K_FUNC_COS, 0); break;
            case 'T': send(e, EQW_K_FUNC_TAN, 0); break;
            case 'n': send(e, EQW_K_FUNC_LN, 0); break;
            case 'X': send(e, EQW_K_FUNC_EXP, 0); break;
            case 'g': send(e, EQW_K_FUNC_LOG, 0); break;
            case 'a': send(e, EQW_K_FUNC_ABS, 0); break;
            default:
                fprintf(stderr, "unknown escape $%c\n", esc);
                return -1;
            }
            continue;
        }
        switch (c) {
        case '+': send(e, EQW_K_PLUS, 0); break;
        case '-': send(e, EQW_K_MINUS, 0); break;
        case '*': send(e, EQW_K_MUL, 0); break;
        case '/': send(e, EQW_K_DIV, 0); break;
        case '^': send(e, EQW_K_POW, 0); break;
        case '@': send(e, EQW_K_SQRT, 0); break;
        case '#': send(e, EQW_K_NTHROOT, 0); break;
        case '(': send(e, EQW_K_PAREN, 0); break;
        case '|': send(e, EQW_K_ABS, 0); break;
        case 'I': send(e, EQW_K_INTEG, 0); break;
        case 'S': send(e, EQW_K_SUM, 0); break;
        case 'D': send(e, EQW_K_DERIV, 0); break;
        case 'W': send(e, EQW_K_WHERE, 0); break;
        case 'C': send(e, EQW_K_CMPLX, 0); break;
        case 'U': send(e, EQW_K_USERFUNC, 0); break;
        case ',': send(e, EQW_K_COMMA, 0); break;
        case ' ': send(e, EQW_K_COMMA, 0); break;
        default:
            if (isalnum((unsigned char)c) || c == '.' || c == '_') {
                send(e, EQW_K_CHAR, (unsigned char)c);
            } else {
                fprintf(stderr, "unknown char '%c' (0x%02x)\n", c, (unsigned)(unsigned char)c);
                return -1;
            }
            break;
        }
    }
    return 0;
}

extern const testcase_t TESTS[];
extern const int NTESTS;

int main(int argc, char **argv)
{
    int verbose = 0;
    const char *outfile = NULL;
    int filter_idx = -1;
    const char *filter_cat = NULL;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-v")) verbose = 1;
        else if (!strcmp(argv[i], "-o") && i + 1 < argc) outfile = argv[++i];
        else if (!strcmp(argv[i], "-i") && i + 1 < argc) filter_idx = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-c") && i + 1 < argc) filter_cat = argv[++i];
    }

    FILE *out = NULL;
    if (outfile) {
        out = fopen(outfile, "w");
        if (!out) { perror(outfile); return 1; }
    }

    int pass = 0, fail = 0;
    int last_cat_pass = 0, last_cat_fail = 0;
    const char *last_cat = NULL;

    if (out) {
        fprintf(out, "# sdleqw vs HP 50g EQW behavioral validation\n\n");
        fprintf(out, "Each row is a single test. The keystroke mini-language is documented in `src/test_harness.c`.\n\n");
        fprintf(out, "| # | category | name | keys | expected (canonical) | sdleqw output | result |\n");
        fprintf(out, "|---|---|---|---|---|---|---|\n");
    }

    for (int i = 0; i < NTESTS; i++) {
        const testcase_t *t = &TESTS[i];
        if (filter_idx >= 0 && i != filter_idx) continue;
        if (filter_cat && strcmp(filter_cat, t->category) != 0) continue;

        if (last_cat && strcmp(last_cat, t->category) != 0) {
            if (verbose) {
                printf("--- %s: %d pass, %d fail ---\n", last_cat, last_cat_pass, last_cat_fail);
            }
            last_cat_pass = last_cat_fail = 0;
        }
        last_cat = t->category;

        eqw_t *e = eqw_new();
        if (t->starter) {
            expr *r = t->starter();
            eqw_load(e, r);
        }
        int rc = run_seq(e, t->keys);
        char *got = expr_to_string(e->root);
        int ok = (rc == 0) && (strcmp(got, t->expected) == 0);
        if (ok) { pass++; last_cat_pass++; }
        else    { fail++; last_cat_fail++; }

        if (verbose || !ok) {
            printf("%s [%s] %s (%-22s keys=%-30s want=%-30s got=%s)\n",
                   ok ? "PASS" : "FAIL",
                   t->category, t->name,
                   t->name, t->keys, t->expected, got);
        }
        if (out) {
            fprintf(out, "| %d | %s | %s | `%s` | `%s` | `%s` | %s |\n",
                    i, t->category, t->name, t->keys, t->expected, got,
                    ok ? "PASS" : "FAIL");
        }
        free(got);
        eqw_free(e);
    }
    if (verbose && last_cat) {
        printf("--- %s: %d pass, %d fail ---\n", last_cat, last_cat_pass, last_cat_fail);
    }
    printf("Total: %d pass, %d fail (%d tests)\n", pass, fail, NTESTS);
    if (out) {
        fprintf(out, "\n**Total: %d pass, %d fail of %d tests.**\n", pass, fail, NTESTS);
        fclose(out);
    }
    return fail ? 1 : 0;
}
