# sdleqw — HP 50g Equation Writer in pure C + SDL

A from-scratch reimplementation of the HP 50g calculator's *Equation Writer*
(EQW) — the textbook-style 2D math editor — in portable C, rendering into a
1-bpp 131×80 pixel buffer and presenting it through SDL2 at 4× scale.

The behavioral spec is the **Meta Kernel** documentation (chapter 8: "Equation
editor") and the **HP 50g User's Guide** chapter 2 (Equation Writer
walkthroughs). The HP 50g LCD is 131×80, the system stack font is 5×7 and the
mini font is 4×6 — `sdleqw` reproduces those faithfully.

## What's in the box

| Module               | Purpose                                                            |
| -------------------- | ------------------------------------------------------------------ |
| `src/bitmap.[ch]`    | 1-bpp framebuffer + drawing primitives                             |
| `src/font.[ch]`      | hand-drawn 5×7 stack font, 4×6 mini font, and procedural tall glyphs (∫, Σ, √, abs-bars, parens) that scale with content height |
| `src/expr.[ch]`      | tagged-tree of math nodes with parent links (NUM / NAME / +,−,·,/,^, √, ⁿ√, abs, parens, prefix-fn, user-fn, ∂, ∫, Σ, where, complex, =) |
| `src/render.[ch]`    | two-pass measure / place-and-draw with auto-parenthesization by precedence and selection highlighting |
| `src/eqw.[ch]`       | three-mode (selection / edit / cursor) state machine and key dispatch matching the Meta Kernel's documented EQW behavior |
| `src/main.c`         | SDL2 frontend at 4× scale plus a headless `--render` mode         |

## Build

Requires `libsdl2-dev`, `pkg-config`, and a C99 compiler. Then:

```sh
make
./sdleqw                         # blank EQW, type to enter your own equation
./sdleqw --demo 0                # E = ∫₀^{1/X} |F(t)| dt   (Meta Kernel doc)
./sdleqw --demo 1                # ∂x ( cos x / sin² x )    (Meta Kernel doc)
./sdleqw --demo 2                # 5·(1+1/7.5)/(√3 − 2³)    (User Guide)
./sdleqw --demo 3                # 2L·√(1+x/R)/(R+y) + 2L/b (User Guide)
./sdleqw --demo 4                # Σ_{k=1}^n k²

./sdleqw --demo 0 --render demo0.pgm   # headless: write a 131×80 PGM
```

## Key bindings

EQW historically remaps the calculator's keyboard. On a PC keyboard the bindings are:

| Key                         | EQW action                                                |
| --------------------------- | --------------------------------------------------------- |
| `0–9`, `.`, `_`             | enter NUM (numeric literal)                                |
| `a–z`, `A–Z`                | enter NAME (identifier) — except for the chord-keys below |
| `+ − * /`                   | binary ADD / SUB / MUL / DIV (DIV draws a stacked bar)   |
| `^`                         | exponent (POW), exp drawn in mini font as superscript     |
| `'`                         | wrap in explicit parens                                   |
| `\|`                        | wrap in absolute-value bars                               |
| `,` or SPACE                | move to next argument slot                                |
| `q`                         | √ (square root)                                           |
| `Q`                         | ⁿ√ (nth-root, places selection under the radical)         |
| `i`                         | ∫ — integral with placeholder lo / hi / body / var         |
| `s`                         | Σ — summation                                             |
| `d`                         | ∂ — partial derivative                                    |
| `w`                         | "where" function (body \| var = value)                    |
| `c`                         | complex number `(re,im)`                                  |
| `u`                         | promote NAME under cursor to a user function              |
| `S`,`C`,`T`,`L`,`E`,`G`,`A` | apply prefix function SIN, COS, TAN, LN, EXP, LOG, ABS    |
| `TAB`                       | toggle selection ↔ edit mode on the current node          |
| `BACKSPACE`                 | replace selection with empty placeholder, enter edit mode |
| `Ctrl+D`                    | delete a 1-arg function (8.3.5)                            |
| `Ctrl+C`                    | delete one argument of a two-arg function (8.3.5)         |
| `Ctrl+L`                    | delete all but the selection                              |
| `z` or `F2`                 | toggle stack font ↔ mini font (zoom)                      |
| `k` or `F3`                 | enter cursor mode (free-roaming caret)                    |
| `F1`                        | load demo expression                                      |
| Arrow keys                  | navigate selection (or move cursor in cursor mode)        |
| `ENTER`                     | commit and exit, prints linear form on stdout             |
| `ESC`                       | cancel and exit                                           |

Cursor mode (per docs §8.4): arrow keys roam a free caret over the rendered
formula, `ENTER` selects the boxed sub-expression, `ESC` returns without
changes. Hold Shift for fast (multi-pixel) movement.

## Architecture notes

**1-bpp bitmap.** The HP 50g LCD is 131 columns × 80 rows of 4-grayscale
pixels; the system uses primarily monochrome rendering, so we model it as
1-bpp with an MSB-left bit order and a `(w+7)/8` byte stride. All drawing is
direct pixel manipulation — no external graphics library involvement.

**Hand-rolled fonts.** The 5×7 stack font and 4×6 mini font are written
in-place with a `R5(b0,b1,b2,b3,b4)` macro for legibility. ASCII 32–126 plus a
private codepoint range (128+: ·, π, θ, λ, μ, Δ, ε, ∞, ∂, …) are covered.

**Procedural tall glyphs.** Parentheses, abs-bars, ∫, Σ, and the √ hook all
scale with their content. `glyph_draw_paren_l(bm, x, y, h, v)` switches
between the rasterized `(` glyph (for short content) and a hand-traced curved
two-pixel-wide tall paren (for fractions, integrals, etc.).

**Two-pass renderer.** `render_layout` walks the tree post-order computing
`(w, h, axis)` per node — `axis` is the y-offset where the math centerline
lives. `place_and_draw` runs pre-order, positioning each child so its axis
aligns with the parent's axis, then drawing the parent's own decoration
(operator symbol, vinculum, parens, integral hooks, etc.) and recursing.

**Auto-parenthesization.** `needs_parens(parent_kind, child_idx, child_kind)`
codifies precedence: ADD/SUB don't paren their MUL/DIV children; MUL parens
ADD/SUB children; POW parens any base of lower precedence than POW (and
always parens a NEG base); DIV / SQRT / ABS / FUNC suppress parens because
the geometric grouping already disambiguates.

**Selection / edit / cursor.** The state machine in `eqw.c` exactly matches
the three documented modes:

* *Selection* — a sub-expression is highlighted (inverted rectangle); F-keys
  apply functions; arrow keys walk siblings / ancestors / first-child;
  digit/letter replaces the selection with a fresh edit token.
* *Edit* — a flashing caret appears at the right edge of a NUM/NAME node;
  characters append; binary operators commit the edit and wrap the now-
  finalized token.
* *Cursor* — arrow keys roam a 3×3 caret freely; on ENTER, the sub-expression
  geometrically under the caret becomes the new selection.

## Behavioral references vs. the real EQW

This is a *replica* of EQW's user-visible behavior, **not** a port of HP's
firmware (the actual EQW is a closed-source ROM module). The reference
emulator at `~/github/x50ng` runs the original ARM firmware in QEMU; it
supplied the LCD geometry (131×80) and the font sizes (5×7 / 4×6) used here.
The semantic behavior — three modes, key responses, error messages, the
auto-parenthesization rules — comes from the **Meta Kernel** documentation
(chapter 8) and the **HP 50g User's Guide** chapter 2.

A few EQW features documented in the spec are stubbed in the state machine
but not visually polished: COLCT / EVAL / EXPAND application, the SUB/REPL
copy-paste pair, and the explicit "First Argument Missing" / "Algebraic
Expected" / "Command Forbidden" error overlays. The expression renderer
itself handles every node type listed in §8.2.

## Headless screenshot

```sh
./sdleqw --demo 0 --render demo0.pgm
pnmtopng < demo0.pgm > demo0.png
```

The output is a 131×80 PGM whose foreground pixels are `0x1A` and background
pixels are `0xC0` — close to the LCD's actual contrast ratio.

## License

GPL-2.0-only, matching the parent repo.
