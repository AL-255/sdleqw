# Demo: step-by-step ABX comparison

This document walks the **demo_arith** expression — `5·(1 + 1/7.5) / (sqrt(3) − 2^3)` — keystroke by keystroke, comparing **sdleqw** against **HP x50ng** for every intermediate state.

The keystroke string is `5*1+1/7.5$r/@3-2^3` (17 atomic actions, 18 snapshots counting the empty-EQW init state). After each keypress we capture the LCD bitmap from both emulators and pixel-diff them.

## How to reproduce

```bash
# 1. generate the per-keystroke scripts
python3 tools/gen_demo_step.py

# 2. run x50ng (writes to /tmp/demo_step/hp/h_NNN_*.pgm)
bash tools/run_demo_step_x50ng.sh

# 3. run sdleqw (writes to /tmp/demo_step/sdl/s_NNNN_*.pgm)
./sdleqw --batch scripts/demo_step.script --snap-prefix /tmp/demo_step/sdl/s

# 4. diff each step
python3 tools/diff_demo_step.py

# 5. side-by-side composite images
bash tools/abx_demo_step.sh    # → abx/demo_step/<step>.png
```

## Final scores

| step | keys                  | tree after step                                                | diff |
|------|-----------------------|----------------------------------------------------------------|-----:|
| 00   | (init, blank EQW)     | `?`                                                            |   19 |
| 01   | `5`                   | `5`                                                            |    0 |
| 02   | `*`                   | `5·?`                                                          |    0 |
| 03   | `1`                   | `5·1`                                                          |    0 |
| 04   | `+`                   | `5·1 + ?`                                                      |    0 |
| 05   | `1`                   | `5·1 + 1`                                                      |    0 |
| 06   | `/`                   | `5·1 + 1/?`                                                    |    0 |
| 07   | `7`                   | `5·1 + 1/7`                                                    |    0 |
| 08   | `.`                   | `5·1 + 1/7.`                                                   |    0 |
| 09   | `5`                   | `5·1 + 1/7.5`                                                  |    0 |
| 10   | `$r` (right)          | `5·1 + 1/7.5` with sel=DIV[1, 7.5]                             |  323 |
| 11   | `/`                   | `5·1 + (1/7.5)/?`                                              |    0 |
| 12   | `@` (sqrt)            | `5·1 + (1/7.5)/sqrt(?)`                                        |    0 |
| 13   | `3`                   | `5·1 + (1/7.5)/sqrt(3)`                                        |    0 |
| 14   | `-`                   | `5·1 + (1/7.5)/sqrt(3 − ?)`                                    |    0 |
| 15   | `2`                   | `5·1 + (1/7.5)/sqrt(3 − 2)`                                    |    0 |
| 16   | `^`                   | `5·1 + (1/7.5)/sqrt(3 − 2^?)`                                  |    0 |
| 17   | `3`                   | `5·1 + (1/7.5)/sqrt(3 − 2^3)`                                  |    0 |

**16/18 steps are pixel-perfect (0 diff).** The two non-zero steps are non-deterministic blink-phase artifacts of HP's x50ng snapshot timing — explained below.

## Issues found and fixed

Each fix was driven by inspecting the diff at a single step and adjusting layout/centering rules. After each change we re-ran both the per-step demo diff and the full 516-test parity suite; numbers below report the demo-step diff and the full-suite (perfect / total px) impact.

### 1. Step 02 (`5*`) — content shifted left by 2 cols
HP centers the visible content as if a trailing empty placeholder collapsed to a 1-col cursor slot. We were including the placeholder's full 5-col layout width, shifting the centering 2 cols left.

**Fix** (`src/render.c` rmost-trailing switch): when the rightmost element of the binary chain is `EX_PLACEHOLDER`, subtract 4 from the centering reserve. *(02_mul, 04_plus 0 px)*

### 2. Step 06 (`5*1+1/`) — bar 1 col left, content shifted
After the trailing placeholder is hidden inside a `DIV`, the cursor lives in the denominator slot — there's no need for the +4 right-side cursor reserve at the root.

**Fix**: when the rightmost binary chain ends at `EX_DIV` / `EX_INTEG` / `EX_SUM`, set centering reserve to 0 (cursor lives inside the structure). *(06_div, 07_7 0 px; full suite +1 perfect)*

### 3. Step 06 (continued) — DIV bar with empty placeholder denominator was 13 cols
HP's min-13 floor on the DIV bar only applies when the denominator has content. With an empty placeholder denominator HP collapses the bar to just-enough-for-the-cursor (≈ 7 cols here).

**Fix**: skip the min-13 floor when `d->kind == EX_PLACEHOLDER`. *(06_div, 11_div 0 px)*

### 4. Step 09 (`5*1+1/7.5`) — bar 1 col too wide / equation 1 col left
The DIV cursor reserve `+9` was too generous for atoms ending in normal-width glyphs (like `5`). HP uses `+8` when the trailing glyph is normal width and `+10` when it's narrow (like `.`).

**Fix**: split the cursor-reserve constant by trailing-glyph width: `last_w < 4 ? +10 : +8`. *(09_5 0 px)*

### 5. Step 12 (`…@`) — SQRT[placeholder] bar was 6 cols too wide
We always reserved 8 cols of right-side cursor space inside SQRT. When the body is just an empty placeholder (or any structure whose rightmost child is a placeholder) the cursor lives in the placeholder slot — no extra reserve needed.

**Fix**: in SQRT layout, walk to the body's rightmost descendant; if that's an `EX_PLACEHOLDER`, use `rpad = 2` instead of 8. Also use `rpad = 2` directly when the body kind is `EX_PLACEHOLDER`. *(12_sqrt 0 px)*

### 6. Step 14 (`…@3-`) — SQRT[SUB[3, _]] body too wide; whole equation 3 cols left
Same root cause as #5 but with a compound body. The trailing placeholder of `SUB[3, _]` should let SQRT skip its cursor reserve.

**Fix**: same walk-rightmost rule as #5. *(14_minus 0 px; 16_pow 0 px as a side effect)*

### 7. DIV denominator centering for structured bodies
When the DIV denominator is a `SQRT` (or other structure whose cursor lives inside), the standard `(e->w − d_adv − 4) / 2` over-reserves on the right. We center without the −4 cursor reserve in that case.

**Fix** (`src/render.c` EX_DIV draw path): only subtract the 4-col cursor reserve when centering an atomic den (`EX_NUM` / `EX_NAME` / `EX_PLACEHOLDER`); for structured dens, center using `(e->w − d_adv) / 2`. *(13_3 0 px)*

### 8. min-13 floor on DIV bar
HP's min-13 floor is only applied while the user is editing inside the DIV. Once they navigate away (mode = SEL with selection elsewhere), the bar collapses to its natural content width.

**Fix**: gate min-13 on a new `edit_in_div` check that walks `ctx->edit`'s parent chain looking for this DIV. *(several `…$u` tests improved; full suite +200 px reduction)*

## Remaining diffs (blink-phase, non-deterministic)

### Step 00 (`init`) — 19 px

HP's snapshot of the empty EQW shows the static cursor wedge inside the placeholder; sdleqw's batch render does not. Adding the wedge unconditionally to root placeholders fixes this step but regresses two other tests (`edit_bksp_one`, `edit_double_bksp`) where HP captured the cursor in its blink-OFF phase. Net: leaving 00_init at 19 px is the better trade-off.

### Step 10 (`$r`) — 323 px

After right-arrow, sel = DIV[1, 7.5] in SEL mode. HP's snapshot caught the **blink-on** phase: the entire `1/7.5` sub-expression appears as inverse-video (filled-black with text-shaped white holes). sdleqw's batch render uses `caret_visible = 0`, suppressing the inverse box.

We can't paint the inverse box unconditionally in batch — many other `…$u`/`…$r` tests caught HP's blink-OFF phase, where the box isn't there. Drawing it always regresses ~20 SEL-mode tests far more than it gains step 10. Net: this 323-px diff is purely a snapshot-timing artifact, not a real visual divergence — sdleqw's interactive mode draws the inverse box correctly when caret is on.

## Summary

| metric                            | before | after  |
|-----------------------------------|-------:|-------:|
| demo_arith 0-px steps             |  3/18  | 16/18  |
| demo_arith total px diff          |  2122  |   342  |
| full-suite perfect tests          | 290/516| 291/516|
| full-suite total px diff          | 23518  | 22995  |

Both improvements are concentrated in atomic and small-structure layout: trailing-placeholder centering, DIV/SQRT cursor reserves, min-13 floor gating. The two remaining demo-step diffs are timing artifacts from HP's snapshot capture and have no fix that doesn't regress other tests.
