#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "bitmap.h"
#include "expr.h"
#include "render.h"
#include "eqw.h"
#include "font.h"

#define LCD_W 131
#define LCD_H 80
#define SCALE 4

#define WIN_W (LCD_W * SCALE)
#define WIN_H (LCD_H * SCALE + 80)   /* extra strip for hints */

#define COLOR_OFF 0xFFB5BD8C        /* LCD background */
#define COLOR_ON  0xFF1A1F1A        /* LCD foreground */
#define COLOR_BG  0xFF202020
#define COLOR_HINT 0xFFCCCCCC

static const char *KEY_HINTS[] = {
    "  digits/letters: enter NUM/NAME",
    "  +  -  *  /  : binary ops",
    "  ^ : power     ' : parens",
    "  | : abs       , : next arg",
    "  q : sqrt      Q : nth-root",
    "  i : integral  s : sum",
    "  d : deriv     w : where",
    "  c : complex   u : userfunc",
    "  S : SIN       C : COS  T : TAN",
    "  L : LN        E : EXP  G : LOG",
    "  TAB: edit/sel BackSp: del-sel",
    "  Ctrl+D: forward-del 1-arg fn",
    "  Ctrl+C: clear-arg of 2-arg fn",
    "  Ctrl+L: clear-all-but-sel",
    "  z: zoom (mini font toggle)",
    "  k: cursor mode",
    "  F1: load demo expression",
    "  Esc: quit       Enter: commit",
    NULL,
};

/* Convert 1-bpp bitmap to 32-bit RGBA buffer (premultiplied alpha not needed). */
static void blit_lcd(const bitmap_t *bm, uint32_t *out)
{
    for (int y = 0; y < bm->h; y++) {
        for (int x = 0; x < bm->w; x++) {
            out[y * bm->w + x] = bm_get(bm, x, y) ? COLOR_ON : COLOR_OFF;
        }
    }
}

/* SDL → eqw_key translation. Returns 1 if a key was produced. */
static int translate_key(SDL_Keysym ks, eqw_key *out)
{
    memset(out, 0, sizeof(*out));
    int shift = (ks.mod & KMOD_SHIFT) ? 1 : 0;
    int ctrl = (ks.mod & KMOD_CTRL) ? 1 : 0;
    out->shift = shift;

    SDL_Keycode kc = ks.sym;

    /* Ctrl combos for the 'second-stick' deletion variants per docs. */
    if (ctrl) {
        switch (kc) {
        case SDLK_d: out->k = EQW_K_DEL; return 1;
        case SDLK_c: out->k = EQW_K_CLEAR; return 1;
        case SDLK_l: out->k = EQW_K_CLEAR; out->shift = 1; return 1;
        }
    }

    switch (kc) {
    case SDLK_LEFT:    out->k = EQW_K_LEFT; return 1;
    case SDLK_RIGHT:   out->k = EQW_K_RIGHT; return 1;
    case SDLK_UP:      out->k = EQW_K_UP; return 1;
    case SDLK_DOWN:    out->k = EQW_K_DOWN; return 1;
    case SDLK_RETURN:  out->k = EQW_K_ENTER; return 1;
    case SDLK_ESCAPE:  out->k = EQW_K_ESC; return 1;
    case SDLK_TAB:     out->k = EQW_K_TAB; return 1;
    case SDLK_BACKSPACE: out->k = EQW_K_BACKSPACE; return 1;
    case SDLK_DELETE:  out->k = EQW_K_DEL; return 1;
    case SDLK_F1:      out->k = EQW_K_DEMO; return 1;
    case SDLK_F2:      out->k = EQW_K_ZOOM; return 1;
    case SDLK_F3:      out->k = EQW_K_CURSOR; return 1;
    }

    /* Single-char keys (use SDL text input ideally; we approximate). */
    if (kc >= 32 && kc < 127) {
        char c = (char)kc;
        if (shift && isalpha(c)) c = (char)toupper(c);
        else if (!shift && isalpha(c)) c = (char)tolower(c);

        switch (c) {
        case '+': out->k = EQW_K_PLUS; return 1;
        case '-': out->k = EQW_K_MINUS; return 1;
        case '*': out->k = EQW_K_MUL; return 1;
        case '/': out->k = EQW_K_DIV; return 1;
        case '^': out->k = EQW_K_POW; return 1;
        case '\'': out->k = EQW_K_PAREN; return 1;
        case '|': out->k = EQW_K_ABS; return 1;
        case ',': out->k = EQW_K_COMMA; return 1;
        case ' ': out->k = EQW_K_COMMA; return 1;
        case 'q': out->k = EQW_K_SQRT; return 1;
        case 'Q': out->k = EQW_K_NTHROOT; return 1;
        case 'i': out->k = EQW_K_INTEG; return 1;
        case 's': out->k = EQW_K_SUM; return 1;
        case 'd': out->k = EQW_K_DERIV; return 1;
        case 'w': out->k = EQW_K_WHERE; return 1;
        case 'c': out->k = EQW_K_CMPLX; return 1;
        case 'u': out->k = EQW_K_USERFUNC; return 1;
        case 'z': out->k = EQW_K_ZOOM; return 1;
        case 'k': out->k = EQW_K_CURSOR; return 1;
        case 'S': out->k = EQW_K_FUNC_SIN; return 1;
        case 'C': out->k = EQW_K_FUNC_COS; return 1;
        case 'T': out->k = EQW_K_FUNC_TAN; return 1;
        case 'L': out->k = EQW_K_FUNC_LN; return 1;
        case 'E': out->k = EQW_K_FUNC_EXP; return 1;
        case 'G': out->k = EQW_K_FUNC_LOG; return 1;
        case 'A': out->k = EQW_K_FUNC_ABS; return 1;
        }

        /* otherwise treat as a generic char insertion */
        if (isdigit(c) || isalpha(c) || c == '.' || c == '_') {
            out->k = EQW_K_CHAR;
            out->ch = c;
            return 1;
        }
    }
    return 0;
}

/* Headless render: build expression N, render once to a 131x80 PGM. */
static int render_to_pgm(const char *path, int demo_idx)
{
    eqw_t *e = eqw_new();
    expr *root = eqw_demo_expr_n(demo_idx);
    eqw_load(e, root);
    bitmap_t *bm = bm_new(LCD_W, LCD_H);
    eqw_render(e, bm, 0);
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); return 1; }
    fprintf(f, "P5\n%d %d\n255\n", LCD_W, LCD_H);
    for (int y = 0; y < LCD_H; y++) {
        for (int x = 0; x < LCD_W; x++) {
            uint8_t v = bm_get(bm, x, y) ? 0x1A : 0xC0;
            fputc(v, f);
        }
    }
    fclose(f);
    bm_free(bm);
    char *s = expr_to_string(e->root);
    fprintf(stderr, "demo %d expression: %s\n", demo_idx, s);
    free(s);
    eqw_free(e);
    fprintf(stderr, "wrote %s (%dx%d)\n", path, LCD_W, LCD_H);
    return 0;
}

/* Headless render of an arbitrary expression to PGM, for debug. */
static int render_expr_to_pgm(expr *root, const char *path)
{
    eqw_t *e = eqw_new();
    eqw_load(e, root);
    bitmap_t *bm = bm_new(LCD_W, LCD_H);
    eqw_render(e, bm, 0);
    FILE *f = fopen(path, "wb");
    if (!f) return 1;
    fprintf(f, "P5\n%d %d\n255\n", LCD_W, LCD_H);
    for (int y = 0; y < LCD_H; y++)
        for (int x = 0; x < LCD_W; x++)
            fputc(bm_get(bm, x, y) ? 0x1A : 0xC0, f);
    fclose(f);
    bm_free(bm);
    eqw_free(e);
    return 0;
}

/* Draw the side hint text on the SDL surface using a tiny custom fallback. */
static void draw_text_software(uint32_t *fb, int fbw, int fbh,
                               int x, int y, const char *s, uint32_t color,
                               int scale)
{
    while (*s) {
        const glyph_t *g = font_stack((unsigned char)*s++);
        for (int j = 0; j < g->h; j++) {
            uint8_t r = g->rows[j];
            for (int i = 0; i < g->w; i++) {
                if (!(r & (1u << i))) continue;
                for (int sy = 0; sy < scale; sy++) {
                    for (int sx = 0; sx < scale; sx++) {
                        int px = x + i * scale + sx;
                        int py = y + j * scale + sy;
                        if (px < 0 || py < 0 || px >= fbw || py >= fbh) continue;
                        fb[py * fbw + px] = color;
                    }
                }
            }
        }
        x += g->advance * scale;
    }
}

int main(int argc, char **argv)
{
    int headless = 0;
    const char *out_path = NULL;
    int demo = 0;
    int demo_idx = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--render") && i + 1 < argc) {
            out_path = argv[++i];
            headless = 1;
        } else if (!strcmp(argv[i], "--demo") && i + 1 < argc &&
                   argv[i+1][0] >= '0' && argv[i+1][0] <= '9') {
            demo = 1;
            demo_idx = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--demo")) {
            demo = 1;
        } else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
            fprintf(stderr,
                "Usage: %s [--demo [N]] [--render path.pgm]\n"
                "  --demo [N]   start with demo N loaded (0..%d)\n"
                "  --render P   headless: render selected demo to P (PGM)\n",
                argv[0], eqw_demo_count() - 1);
            return 0;
        }
    }

    if (headless) return render_to_pgm(out_path, demo_idx);

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Window *win = SDL_CreateWindow("HP50g EQW (sdleqw)",
                                       SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                       WIN_W, WIN_H, SDL_WINDOW_SHOWN);
    if (!win) { fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError()); return 1; }
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) ren = SDL_CreateRenderer(win, -1, 0);
    if (!ren) { fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError()); return 1; }

    SDL_Texture *lcd_tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888,
                                             SDL_TEXTUREACCESS_STREAMING, LCD_W, LCD_H);

    bitmap_t *bm = bm_new(LCD_W, LCD_H);
    eqw_t *e = eqw_new();
    if (demo) eqw_load(e, eqw_demo_expr_n(demo_idx));

    /* Side panel framebuffer rendered in software (text only). */
    uint32_t *side_fb = malloc((size_t)WIN_W * (WIN_H - LCD_H * SCALE) * 4);
    SDL_Texture *side_tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888,
                                              SDL_TEXTUREACCESS_STREAMING,
                                              WIN_W, WIN_H - LCD_H * SCALE);

    Uint32 last_blink = SDL_GetTicks();
    int caret_visible = 1;

    int running = 1;
    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) running = 0;
            if (ev.type == SDL_KEYDOWN) {
                eqw_key k = { 0 };
                if (translate_key(ev.key.keysym, &k)) {
                    eqw_handle(e, &k);
                    caret_visible = 1;
                    last_blink = SDL_GetTicks();
                    if (e->exit_code) {
                        if (e->exit_code == 1) {
                            char *s = expr_to_string(e->root);
                            printf("ENTER: %s\n", s);
                            fflush(stdout);
                            free(s);
                        } else {
                            printf("ESC: cancelled\n");
                            fflush(stdout);
                        }
                        running = 0;
                    }
                }
            }
        }

        Uint32 now = SDL_GetTicks();
        if (now - last_blink > 500) {
            caret_visible = !caret_visible;
            last_blink = now;
        }

        eqw_render(e, bm, caret_visible);

        /* Update LCD texture. */
        uint32_t lcd_pix[LCD_W * LCD_H];
        blit_lcd(bm, lcd_pix);
        SDL_UpdateTexture(lcd_tex, NULL, lcd_pix, LCD_W * 4);

        /* Update side panel. */
        int sw = WIN_W;
        int sh = WIN_H - LCD_H * SCALE;
        for (int i = 0; i < sw * sh; i++) side_fb[i] = COLOR_BG;
        int ty = 4;
        for (int i = 0; KEY_HINTS[i]; i++) {
            draw_text_software(side_fb, sw, sh, 4, ty, KEY_HINTS[i], COLOR_HINT, 1);
            ty += STACK_FONT_H + 1;
            if (ty > sh - STACK_FONT_H) break;
        }
        SDL_UpdateTexture(side_tex, NULL, side_fb, sw * 4);

        SDL_SetRenderDrawColor(ren, 0x10, 0x10, 0x10, 0xFF);
        SDL_RenderClear(ren);
        SDL_Rect lcd_rect = { 0, 0, LCD_W * SCALE, LCD_H * SCALE };
        SDL_RenderCopy(ren, lcd_tex, NULL, &lcd_rect);
        SDL_Rect side_rect = { 0, LCD_H * SCALE, sw, sh };
        SDL_RenderCopy(ren, side_tex, NULL, &side_rect);
        SDL_RenderPresent(ren);

        SDL_Delay(16);
    }

    free(side_fb);
    SDL_DestroyTexture(lcd_tex);
    SDL_DestroyTexture(side_tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    bm_free(bm);
    eqw_free(e);
    (void)render_expr_to_pgm; /* silence unused warning when not used */
    return 0;
}
