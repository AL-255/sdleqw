/* Headless behavioral test: drive eqw with a scripted key sequence and
   print the resulting linear expression. Used to verify operator wrapping
   matches the documented behavior (8.2.4). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "expr.h"
#include "eqw.h"
#include "bitmap.h"

static void send(eqw_t *e, eqw_keycode kc, int ch)
{
    eqw_key k = { 0 };
    k.k = kc;
    k.ch = ch;
    eqw_handle(e, &k);
}

static void send_chars(eqw_t *e, const char *s)
{
    for (; *s; s++) send(e, EQW_K_CHAR, (unsigned char)*s);
}

static void show(const char *label, eqw_t *e)
{
    char *s = expr_to_string(e->root);
    printf("%-20s -> %s   (mode=%s)\n", label, s, eqw_mode_name(e));
    free(s);
}

int main(void)
{
    int fail = 0;

    /* Test 1: build 123/456, then *789, expect (123/456)*789 */
    {
        eqw_t *e = eqw_new();
        eqw_load(e, expr_binary(EX_DIV, expr_num("123"), expr_num("456")));
        show("loaded 123/456", e);
        send(e, EQW_K_MUL, 0);
        show("after *", e);
        send_chars(e, "789");
        show("after 789", e);

        char *got = expr_to_string(e->root);
        const char *want = "123/456*789";
        if (strcmp(got, want) != 0) {
            fprintf(stderr, "FAIL: want %s, got %s\n", want, got);
            fail = 1;
        } else {
            printf("PASS: %s\n", got);
        }
        free(got);
        eqw_free(e);
    }

    /* Test 2: 5, +2, expect 5+2 */
    {
        eqw_t *e = eqw_new();
        eqw_load(e, expr_num("5"));
        send(e, EQW_K_PLUS, 0);
        send_chars(e, "2");
        char *got = expr_to_string(e->root);
        printf("Test 2: %s (want 5+2)\n", got);
        if (strcmp(got, "5+2") != 0) fail = 1;
        free(got); eqw_free(e);
    }

    /* Test 3: X, ^2, expect X^2 */
    {
        eqw_t *e = eqw_new();
        eqw_load(e, expr_name("X"));
        send(e, EQW_K_POW, 0);
        send_chars(e, "2");
        char *got = expr_to_string(e->root);
        printf("Test 3: %s (want X^2)\n", got);
        if (strcmp(got, "X^2") != 0) fail = 1;
        free(got); eqw_free(e);
    }

    /* Test 4: 5/3, then sqrt, expect SQRT(5/3) */
    {
        eqw_t *e = eqw_new();
        eqw_load(e, expr_binary(EX_DIV, expr_num("5"), expr_num("3")));
        send(e, EQW_K_SQRT, 0);
        char *got = expr_to_string(e->root);
        printf("Test 4: %s (want SQRT(5/3))\n", got);
        if (strcmp(got, "SQRT(5/3)") != 0) fail = 1;
        free(got); eqw_free(e);
    }

    /* Test 5: 5, +, 2, *, 3. Per 8.2.4, "*" applies to the *edited* token
       (the "2"), not the whole expression. So the AST should be
       ADD(5, MUL(2, 3)), printed as "5+2*3". */
    {
        eqw_t *e = eqw_new();
        eqw_load(e, expr_num("5"));
        send(e, EQW_K_PLUS, 0);
        send_chars(e, "2");
        send(e, EQW_K_MUL, 0);
        send_chars(e, "3");
        char *got = expr_to_string(e->root);
        const char *want = "5+2*3";
        printf("Test 5: %s (want %s)\n", got, want);
        if (strcmp(got, want) != 0) fail = 1;
        free(got); eqw_free(e);
    }

    /* Test 6: render Test 1 to a PGM so we can visually confirm. */
    {
        eqw_t *e = eqw_new();
        eqw_load(e, expr_binary(EX_DIV, expr_num("123"), expr_num("456")));
        send(e, EQW_K_MUL, 0);
        send_chars(e, "789");
        bitmap_t *bm = bm_new(131, 80);
        eqw_render(e, bm, 0);
        FILE *f = fopen("/tmp/test1.pgm", "wb");
        fprintf(f, "P5\n131 80\n255\n");
        for (int y = 0; y < 80; y++)
            for (int x = 0; x < 131; x++)
                fputc(bm_get(bm, x, y) ? 0x1A : 0xC0, f);
        fclose(f);
        bm_free(bm);
        eqw_free(e);
        printf("Test 6: wrote /tmp/test1.pgm\n");
    }

    return fail;
}
