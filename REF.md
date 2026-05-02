# sdleqw vs x50ng (HP 50g firmware) — functional validation

This file is the reference comparison between **sdleqw** (this repository's pure-C reimplementation of the HP 50g Equation Writer) and **x50ng** (the official HP49g+/50g hardware emulator running the genuine HP firmware ROM).

## Setup

- **x50ng** built locally from `~/github/x50ng/` with a small patch in `src/main.c` that adds a headless scripting hook (`X50NG_SCRIPT`, `X50NG_SNAP_PREFIX` env vars). The patch ([diff](#x50ng-patch)) drives `press_key()` / `release_key()` from a script and snapshots the LCD via `get_lcd_buffer()`.
- **sdleqw** built normally (`make`) and run with `--batch SCRIPT --snap-prefix PFX`.
- Both binaries run **headlessly** (no SDL window required for x50ng either — it uses the ncurses TUI frontend purely as a host loop, but the LCD bytes come straight from `get_lcd_buffer`).
- The same logical key sequence is fed to each. The mini-language used to author the tests is documented in `tools/gen_tests.py`. For each test, `gen_tests.py` emits an x50ng-flavoured key sequence (with shifts and alpha-locking) and an sdleqw-flavoured key sequence (one EQW action per char).
- Each captured frame is a 131×80 bitmap. The HP LCD buffer comes back as 0..15 grayscale; we threshold to 1-bpp.

## Test cycle

```
# x50ng test cycle:
WAIT 100             # cold boot to home screen
SNAP boot            # baseline
# for each test:
TAP RIGHTSHIFT       # enter EQW: RS+QUOTE
TAP O
WAIT 25
<keys for the test expression>
WAIT 20
SNAP <name>
TAP ON               # cancel EQW back to home
WAIT 15
```

## Results summary

- **Total tests:** 516
- **Both produced a snap:** 516
- x50ng-only: 0,  sdleqw-only: 0,  neither: 0

**Pixel-difference distribution** (binary thresholded; lower = more similar):

Full LCD (131×80) including menu strip:

| bucket | count | share |
|---|---|---|
| <0.5% | 278 | 53.9% |
| 0.5-1% | 101 | 19.6% |
| 1-2% | 119 | 23.1% |
| 2-3% | 13 | 2.5% |
| 3-5% | 4 | 0.8% |
| 5-10% | 1 | 0.2% |
| >10% | 0 | 0.0% |

Equation area only (rows 0..72, menu strip cropped out):

| bucket | count | share |
|---|---|---|
| <0.5% | 269 | 52.1% |
| 0.5-1% | 86 | 16.7% |
| 1-2% | 136 | 26.4% |
| 2-3% | 18 | 3.5% |
| 3-5% | 6 | 1.2% |
| 5-10% | 1 | 0.2% |
| >10% | 0 | 0.0% |

> **What the diff measures.** Each PGM is binary-thresholded; we count the
> pixels that disagree between HP firmware and sdleqw, divided by total
> pixels in the comparison region. 0% = pixel-perfect match. The diff
> is a ranking signal, not a pass/fail. 

> **How sdleqw was tuned to close the gap.** Iteratively:
> 1. Imported HP's variable-width 5×7 stack font glyphs from x50ng snapshots
>    (`tools/gen_hp_font.py`).
> 2. Imported HP's exact menu-strip pixel layout (131×7 bitmap, `src/hp_menu.c`).
> 3. Adjusted vertical centering to use the (LCD_H - MENU_H)=73-row equation area.
> 4. Adjusted horizontal centering to HP's slightly-left-of-center axis (col 62).
> 5. Removed the spurious 4-pixel minimum-width clamp on glyph layout that
>    was widening narrow glyphs like '1'.
> 6. Added 1-pixel gaps above and below division bars to match HP's spacing.

## Per-test comparisons


### Numbers

| # | name | keys | x50ng (HP firmware) | sdleqw | diff |
|---|---|---|---|---|---|
| 0 | `num_0` | `0` | ![](scripts/snaps/hp/num_0.png) | ![](scripts/snaps/sdl/num_0.png) | 0.0% |
| 1 | `num_1` | `1` | ![](scripts/snaps/hp/num_1.png) | ![](scripts/snaps/sdl/num_1.png) | 0.0% |
| 2 | `num_2` | `2` | ![](scripts/snaps/hp/num_2.png) | ![](scripts/snaps/sdl/num_2.png) | 0.0% |
| 3 | `num_3` | `3` | ![](scripts/snaps/hp/num_3.png) | ![](scripts/snaps/sdl/num_3.png) | 0.0% |
| 4 | `num_4` | `4` | ![](scripts/snaps/hp/num_4.png) | ![](scripts/snaps/sdl/num_4.png) | 0.0% |
| 5 | `num_5` | `5` | ![](scripts/snaps/hp/num_5.png) | ![](scripts/snaps/sdl/num_5.png) | 0.0% |
| 6 | `num_6` | `6` | ![](scripts/snaps/hp/num_6.png) | ![](scripts/snaps/sdl/num_6.png) | 0.0% |
| 7 | `num_7` | `7` | ![](scripts/snaps/hp/num_7.png) | ![](scripts/snaps/sdl/num_7.png) | 0.0% |
| 8 | `num_8` | `8` | ![](scripts/snaps/hp/num_8.png) | ![](scripts/snaps/sdl/num_8.png) | 0.0% |
| 9 | `num_9` | `9` | ![](scripts/snaps/hp/num_9.png) | ![](scripts/snaps/sdl/num_9.png) | 0.0% |
| 10 | `num_10` | `10` | ![](scripts/snaps/hp/num_10.png) | ![](scripts/snaps/sdl/num_10.png) | 0.0% |
| 11 | `num_42` | `42` | ![](scripts/snaps/hp/num_42.png) | ![](scripts/snaps/sdl/num_42.png) | 0.0% |
| 12 | `num_100` | `100` | ![](scripts/snaps/hp/num_100.png) | ![](scripts/snaps/sdl/num_100.png) | 0.0% |
| 13 | `num_1234` | `1234` | ![](scripts/snaps/hp/num_1234.png) | ![](scripts/snaps/sdl/num_1234.png) | 0.0% |
| 14 | `num_12345` | `12345` | ![](scripts/snaps/hp/num_12345.png) | ![](scripts/snaps/sdl/num_12345.png) | 0.0% |
| 15 | `num_3p14` | `3.14` | ![](scripts/snaps/hp/num_3p14.png) | ![](scripts/snaps/sdl/num_3p14.png) | 0.0% |
| 16 | `num_0p5` | `0.5` | ![](scripts/snaps/hp/num_0p5.png) | ![](scripts/snaps/sdl/num_0p5.png) | 0.0% |
| 17 | `num_2p0` | `2.0` | ![](scripts/snaps/hp/num_2p0.png) | ![](scripts/snaps/sdl/num_2p0.png) | 0.0% |
| 18 | `num_7p5` | `7.5` | ![](scripts/snaps/hp/num_7p5.png) | ![](scripts/snaps/sdl/num_7p5.png) | 0.0% |
| 19 | `num_0p001` | `0.001` | ![](scripts/snaps/hp/num_0p001.png) | ![](scripts/snaps/sdl/num_0p001.png) | 0.0% |
| 20 | `num_99p99` | `99.99` | ![](scripts/snaps/hp/num_99p99.png) | ![](scripts/snaps/sdl/num_99p99.png) | 0.0% |
| 21 | `num_1000000` | `1000000` | ![](scripts/snaps/hp/num_1000000.png) | ![](scripts/snaps/sdl/num_1000000.png) | 0.0% |
| 22 | `num_5p0` | `5.0` | ![](scripts/snaps/hp/num_5p0.png) | ![](scripts/snaps/sdl/num_5p0.png) | 0.0% |
| 23 | `num_1p0` | `1.0` | ![](scripts/snaps/hp/num_1p0.png) | ![](scripts/snaps/sdl/num_1p0.png) | 0.0% |
| 24 | `num_25` | `25` | ![](scripts/snaps/hp/num_25.png) | ![](scripts/snaps/sdl/num_25.png) | 0.0% |
| 25 | `num_360` | `360` | ![](scripts/snaps/hp/num_360.png) | ![](scripts/snaps/sdl/num_360.png) | 0.0% |
| 26 | `num_180` | `180` | ![](scripts/snaps/hp/num_180.png) | ![](scripts/snaps/sdl/num_180.png) | 0.0% |
| 27 | `num_2p5` | `2.5` | ![](scripts/snaps/hp/num_2p5.png) | ![](scripts/snaps/sdl/num_2p5.png) | 0.0% |
| 28 | `num_neg_5` | `-5` | ![](scripts/snaps/hp/num_neg_5.png) | ![](scripts/snaps/sdl/num_neg_5.png) | 0.3% |
| 29 | `num_neg_3p14` | `-3.14` | ![](scripts/snaps/hp/num_neg_3p14.png) | ![](scripts/snaps/sdl/num_neg_3p14.png) | 0.8% |
| 30 | `num_dot_5` | `.5` | ![](scripts/snaps/hp/num_dot_5.png) | ![](scripts/snaps/sdl/num_dot_5.png) | 0.0% |
| 31 | `num_99` | `99` | ![](scripts/snaps/hp/num_99.png) | ![](scripts/snaps/sdl/num_99.png) | 0.5% |
| 32 | `num_1000` | `1000` | ![](scripts/snaps/hp/num_1000.png) | ![](scripts/snaps/sdl/num_1000.png) | 0.5% |
| 33 | `num_500` | `500` | ![](scripts/snaps/hp/num_500.png) | ![](scripts/snaps/sdl/num_500.png) | 0.0% |

### Names

| # | name | keys | x50ng (HP firmware) | sdleqw | diff |
|---|---|---|---|---|---|
| 34 | `name_A` | `A` | ![](scripts/snaps/hp/name_A.png) | ![](scripts/snaps/sdl/name_A.png) | 0.0% |
| 35 | `name_B` | `B` | ![](scripts/snaps/hp/name_B.png) | ![](scripts/snaps/sdl/name_B.png) | 0.0% |
| 36 | `name_E` | `E` | ![](scripts/snaps/hp/name_E.png) | ![](scripts/snaps/sdl/name_E.png) | 0.0% |
| 37 | `name_F` | `F` | ![](scripts/snaps/hp/name_F.png) | ![](scripts/snaps/sdl/name_F.png) | 0.0% |
| 38 | `name_G` | `G` | ![](scripts/snaps/hp/name_G.png) | ![](scripts/snaps/sdl/name_G.png) | 0.0% |
| 39 | `name_H` | `H` | ![](scripts/snaps/hp/name_H.png) | ![](scripts/snaps/sdl/name_H.png) | 0.0% |
| 40 | `name_J` | `J` | ![](scripts/snaps/hp/name_J.png) | ![](scripts/snaps/sdl/name_J.png) | 0.0% |
| 41 | `name_K` | `K` | ![](scripts/snaps/hp/name_K.png) | ![](scripts/snaps/sdl/name_K.png) | 0.0% |
| 42 | `name_L` | `L` | ![](scripts/snaps/hp/name_L.png) | ![](scripts/snaps/sdl/name_L.png) | 0.0% |
| 43 | `name_M` | `M` | ![](scripts/snaps/hp/name_M.png) | ![](scripts/snaps/sdl/name_M.png) | 0.0% |
| 44 | `name_N` | `N` | ![](scripts/snaps/hp/name_N.png) | ![](scripts/snaps/sdl/name_N.png) | 0.0% |
| 45 | `name_O` | `O` | ![](scripts/snaps/hp/name_O.png) | ![](scripts/snaps/sdl/name_O.png) | 0.0% |
| 46 | `name_R` | `R` | ![](scripts/snaps/hp/name_R.png) | ![](scripts/snaps/sdl/name_R.png) | 0.0% |
| 47 | `name_T` | `T` | ![](scripts/snaps/hp/name_T.png) | ![](scripts/snaps/sdl/name_T.png) | 0.0% |
| 48 | `name_Y` | `Y` | ![](scripts/snaps/hp/name_Y.png) | ![](scripts/snaps/sdl/name_Y.png) | 0.0% |
| 49 | `name_Z` | `Z` | ![](scripts/snaps/hp/name_Z.png) | ![](scripts/snaps/sdl/name_Z.png) | 0.0% |
| 50 | `name_a` | `a` | ![](scripts/snaps/hp/name_a.png) | ![](scripts/snaps/sdl/name_a.png) | 0.0% |
| 51 | `name_b` | `b` | ![](scripts/snaps/hp/name_b.png) | ![](scripts/snaps/sdl/name_b.png) | 0.0% |
| 52 | `name_e` | `e` | ![](scripts/snaps/hp/name_e.png) | ![](scripts/snaps/sdl/name_e.png) | 0.0% |
| 53 | `name_f` | `f` | ![](scripts/snaps/hp/name_f.png) | ![](scripts/snaps/sdl/name_f.png) | 0.0% |
| 54 | `name_g` | `g` | ![](scripts/snaps/hp/name_g.png) | ![](scripts/snaps/sdl/name_g.png) | 0.0% |
| 55 | `name_h` | `h` | ![](scripts/snaps/hp/name_h.png) | ![](scripts/snaps/sdl/name_h.png) | 0.0% |
| 56 | `name_j` | `j` | ![](scripts/snaps/hp/name_j.png) | ![](scripts/snaps/sdl/name_j.png) | 0.0% |
| 57 | `name_k` | `k` | ![](scripts/snaps/hp/name_k.png) | ![](scripts/snaps/sdl/name_k.png) | 0.0% |
| 58 | `name_l` | `l` | ![](scripts/snaps/hp/name_l.png) | ![](scripts/snaps/sdl/name_l.png) | 0.0% |
| 59 | `name_m` | `m` | ![](scripts/snaps/hp/name_m.png) | ![](scripts/snaps/sdl/name_m.png) | 0.0% |
| 60 | `name_n` | `n` | ![](scripts/snaps/hp/name_n.png) | ![](scripts/snaps/sdl/name_n.png) | 0.0% |
| 61 | `name_o` | `o` | ![](scripts/snaps/hp/name_o.png) | ![](scripts/snaps/sdl/name_o.png) | 0.0% |
| 62 | `name_r` | `r` | ![](scripts/snaps/hp/name_r.png) | ![](scripts/snaps/sdl/name_r.png) | 0.0% |
| 63 | `name_t` | `t` | ![](scripts/snaps/hp/name_t.png) | ![](scripts/snaps/sdl/name_t.png) | 0.0% |
| 64 | `name_y` | `y` | ![](scripts/snaps/hp/name_y.png) | ![](scripts/snaps/sdl/name_y.png) | 0.0% |
| 65 | `name_z` | `z` | ![](scripts/snaps/hp/name_z.png) | ![](scripts/snaps/sdl/name_z.png) | 0.0% |
| 66 | `name_X` | `X` | ![](scripts/snaps/hp/name_X.png) | ![](scripts/snaps/sdl/name_X.png) | 0.0% |
| 67 | `name_x` | `x` | ![](scripts/snaps/hp/name_x.png) | ![](scripts/snaps/sdl/name_x.png) | 0.0% |
| 68 | `name_xy` | `xy` | ![](scripts/snaps/hp/name_xy.png) | ![](scripts/snaps/sdl/name_xy.png) | 0.0% |
| 69 | `name_abc` | `ABC` | ![](scripts/snaps/hp/name_abc.png) | ![](scripts/snaps/sdl/name_abc.png) | 0.0% |
| 70 | `name_t` | `t` | ![](scripts/snaps/hp/name_t.png) | ![](scripts/snaps/sdl/name_t.png) | 0.0% |
| 71 | `name_R` | `R` | ![](scripts/snaps/hp/name_R.png) | ![](scripts/snaps/sdl/name_R.png) | 0.0% |
| 72 | `name_b` | `b` | ![](scripts/snaps/hp/name_b.png) | ![](scripts/snaps/sdl/name_b.png) | 0.0% |
| 73 | `name_PI` | `PI` | ![](scripts/snaps/hp/name_PI.png) | ![](scripts/snaps/sdl/name_PI.png) | 0.0% |
| 74 | `name_OMEGA` | `OMEGA` | ![](scripts/snaps/hp/name_OMEGA.png) | ![](scripts/snaps/sdl/name_OMEGA.png) | 0.0% |
| 75 | `name_THETA` | `THETA` | ![](scripts/snaps/hp/name_THETA.png) | ![](scripts/snaps/sdl/name_THETA.png) | 0.0% |
| 76 | `name_var1` | `var1` | ![](scripts/snaps/hp/name_var1.png) | ![](scripts/snaps/sdl/name_var1.png) | 0.0% |
| 77 | `name_X1` | `X1` | ![](scripts/snaps/hp/name_X1.png) | ![](scripts/snaps/sdl/name_X1.png) | 0.0% |
| 78 | `name_a1` | `a1` | ![](scripts/snaps/hp/name_a1.png) | ![](scripts/snaps/sdl/name_a1.png) | 0.0% |
| 79 | `name_FOO` | `FOO` | ![](scripts/snaps/hp/name_FOO.png) | ![](scripts/snaps/sdl/name_FOO.png) | 0.0% |
| 80 | `name_BAR` | `BAR` | ![](scripts/snaps/hp/name_BAR.png) | ![](scripts/snaps/sdl/name_BAR.png) | 0.0% |
| 81 | `name_PHI` | `PHI` | ![](scripts/snaps/hp/name_PHI.png) | ![](scripts/snaps/sdl/name_PHI.png) | 0.0% |
| 82 | `name_zeta` | `zeta` | ![](scripts/snaps/hp/name_zeta.png) | ![](scripts/snaps/sdl/name_zeta.png) | 0.0% |
| 83 | `name_eta` | `eta` | ![](scripts/snaps/hp/name_eta.png) | ![](scripts/snaps/sdl/name_eta.png) | 0.0% |
| 84 | `name_NUMBER` | `NUMBER` | ![](scripts/snaps/hp/name_NUMBER.png) | ![](scripts/snaps/sdl/name_NUMBER.png) | 0.0% |
| 85 | `name_value` | `value` | ![](scripts/snaps/hp/name_value.png) | ![](scripts/snaps/sdl/name_value.png) | 0.0% |
| 86 | `name_alpha` | `alpha` | ![](scripts/snaps/hp/name_alpha.png) | ![](scripts/snaps/sdl/name_alpha.png) | 0.0% |
| 87 | `name_beta` | `beta` | ![](scripts/snaps/hp/name_beta.png) | ![](scripts/snaps/sdl/name_beta.png) | 0.0% |
| 88 | `name_gamma` | `gamma` | ![](scripts/snaps/hp/name_gamma.png) | ![](scripts/snaps/sdl/name_gamma.png) | 0.0% |
| 89 | `name_delta` | `delta` | ![](scripts/snaps/hp/name_delta.png) | ![](scripts/snaps/sdl/name_delta.png) | 0.0% |
| 90 | `name_long` | `VARIABLE` | ![](scripts/snaps/hp/name_long.png) | ![](scripts/snaps/sdl/name_long.png) | 0.0% |
| 91 | `name_kappa` | `kappa` | ![](scripts/snaps/hp/name_kappa.png) | ![](scripts/snaps/sdl/name_kappa.png) | 0.0% |

### Addition

| # | name | keys | x50ng (HP firmware) | sdleqw | diff |
|---|---|---|---|---|---|
| 92 | `add_1_1` | `1+1` | ![](scripts/snaps/hp/add_1_1.png) | ![](scripts/snaps/sdl/add_1_1.png) | 0.0% |
| 93 | `add_1_2` | `1+2` | ![](scripts/snaps/hp/add_1_2.png) | ![](scripts/snaps/sdl/add_1_2.png) | 0.0% |
| 94 | `add_5_3` | `5+3` | ![](scripts/snaps/hp/add_5_3.png) | ![](scripts/snaps/sdl/add_5_3.png) | 0.0% |
| 95 | `add_X_Y` | `X+Y` | ![](scripts/snaps/hp/add_X_Y.png) | ![](scripts/snaps/sdl/add_X_Y.png) | 0.0% |
| 96 | `add_X_5` | `X+5` | ![](scripts/snaps/hp/add_X_5.png) | ![](scripts/snaps/sdl/add_X_5.png) | 0.0% |
| 97 | `add_5_X` | `5+X` | ![](scripts/snaps/hp/add_5_X.png) | ![](scripts/snaps/sdl/add_5_X.png) | 0.0% |
| 98 | `add_a_b` | `A+B` | ![](scripts/snaps/hp/add_a_b.png) | ![](scripts/snaps/sdl/add_a_b.png) | 0.0% |
| 99 | `add_3_3_3` | `3+3+3` | ![](scripts/snaps/hp/add_3_3_3.png) | ![](scripts/snaps/sdl/add_3_3_3.png) | 0.0% |
| 100 | `add_5_X_2` | `5+X+2` | ![](scripts/snaps/hp/add_5_X_2.png) | ![](scripts/snaps/sdl/add_5_X_2.png) | 0.0% |
| 101 | `add_1p5_2p5` | `1.5+2.5` | ![](scripts/snaps/hp/add_1p5_2p5.png) | ![](scripts/snaps/sdl/add_1p5_2p5.png) | 0.0% |
| 102 | `add_3p14_2p71` | `3.14+2.71` | ![](scripts/snaps/hp/add_3p14_2p71.png) | ![](scripts/snaps/sdl/add_3p14_2p71.png) | 0.0% |
| 103 | `add_X_Y_Z` | `X+Y+Z` | ![](scripts/snaps/hp/add_X_Y_Z.png) | ![](scripts/snaps/sdl/add_X_Y_Z.png) | 0.0% |
| 104 | `add_long` | `1+2+3+4+5` | ![](scripts/snaps/hp/add_long.png) | ![](scripts/snaps/sdl/add_long.png) | 0.0% |
| 105 | `add_two_names` | `FOO+BAR` | ![](scripts/snaps/hp/add_two_names.png) | ![](scripts/snaps/sdl/add_two_names.png) | 0.0% |
| 106 | `add_zero` | `0+5` | ![](scripts/snaps/hp/add_zero.png) | ![](scripts/snaps/sdl/add_zero.png) | 0.0% |
| 107 | `add_decimal` | `0.1+0.2` | ![](scripts/snaps/hp/add_decimal.png) | ![](scripts/snaps/sdl/add_decimal.png) | 0.4% |
| 108 | `add_X_X` | `X+X` | ![](scripts/snaps/hp/add_X_X.png) | ![](scripts/snaps/sdl/add_X_X.png) | 0.0% |
| 109 | `add_t_dt` | `t+dt` | ![](scripts/snaps/hp/add_t_dt.png) | ![](scripts/snaps/sdl/add_t_dt.png) | 0.0% |
| 110 | `add_neg_5_3` | `-5+3` | ![](scripts/snaps/hp/add_neg_5_3.png) | ![](scripts/snaps/sdl/add_neg_5_3.png) | 0.8% |
| 111 | `add_3_neg_5` | `3+-5` | ![](scripts/snaps/hp/add_3_neg_5.png) | ![](scripts/snaps/sdl/add_3_neg_5.png) | 0.7% |
| 112 | `add_long_decimal` | `1.234+5.678` | ![](scripts/snaps/hp/add_long_decimal.png) | ![](scripts/snaps/sdl/add_long_decimal.png) | 0.0% |
| 113 | `add_var_with_index` | `X1+X2` | ![](scripts/snaps/hp/add_var_with_index.png) | ![](scripts/snaps/sdl/add_var_with_index.png) | 0.4% |
| 114 | `add_var_const` | `X+1` | ![](scripts/snaps/hp/add_var_const.png) | ![](scripts/snaps/sdl/add_var_const.png) | 0.0% |
| 115 | `add_var_neg_const` | `X+-1` | ![](scripts/snaps/hp/add_var_neg_const.png) | ![](scripts/snaps/sdl/add_var_neg_const.png) | 0.5% |
| 116 | `add_PI_2` | `PI+2` | ![](scripts/snaps/hp/add_PI_2.png) | ![](scripts/snaps/sdl/add_PI_2.png) | 0.3% |
| 117 | `add_X_PI` | `X+PI` | ![](scripts/snaps/hp/add_X_PI.png) | ![](scripts/snaps/sdl/add_X_PI.png) | 0.0% |
| 118 | `add_E_M` | `E+M` | ![](scripts/snaps/hp/add_E_M.png) | ![](scripts/snaps/sdl/add_E_M.png) | 0.0% |
| 119 | `add_p_q` | `p+q` | ![](scripts/snaps/hp/add_p_q.png) | ![](scripts/snaps/sdl/add_p_q.png) | 0.0% |
| 120 | `add_n_1` | `n+1` | ![](scripts/snaps/hp/add_n_1.png) | ![](scripts/snaps/sdl/add_n_1.png) | 0.0% |
| 121 | `add_2_n` | `2+n` | ![](scripts/snaps/hp/add_2_n.png) | ![](scripts/snaps/sdl/add_2_n.png) | 0.0% |
| 122 | `add_six_terms` | `1+2+3+4+5+6` | ![](scripts/snaps/hp/add_six_terms.png) | ![](scripts/snaps/sdl/add_six_terms.png) | 0.0% |
| 123 | `add_N_M_K` | `N+M+K` | ![](scripts/snaps/hp/add_N_M_K.png) | ![](scripts/snaps/sdl/add_N_M_K.png) | 0.0% |
| 124 | `add_const_var` | `5+L` | ![](scripts/snaps/hp/add_const_var.png) | ![](scripts/snaps/sdl/add_const_var.png) | 0.0% |
| 125 | `add_var_var_var` | `A+B+R` | ![](scripts/snaps/hp/add_var_var_var.png) | ![](scripts/snaps/sdl/add_var_var_var.png) | 0.0% |
| 126 | `add_three_const` | `10+20+30` | ![](scripts/snaps/hp/add_three_const.png) | ![](scripts/snaps/sdl/add_three_const.png) | 0.0% |
| 127 | `add_three_decimals` | `1.1+2.2+3.3` | ![](scripts/snaps/hp/add_three_decimals.png) | ![](scripts/snaps/sdl/add_three_decimals.png) | 0.9% |
| 128 | `add_neg_neg` | `-1+-2` | ![](scripts/snaps/hp/add_neg_neg.png) | ![](scripts/snaps/sdl/add_neg_neg.png) | 0.5% |
| 129 | `add_X_neg_Y` | `X+-Y` | ![](scripts/snaps/hp/add_X_neg_Y.png) | ![](scripts/snaps/sdl/add_X_neg_Y.png) | 0.6% |
| 130 | `add_Y_X` | `Y+X` | ![](scripts/snaps/hp/add_Y_X.png) | ![](scripts/snaps/sdl/add_Y_X.png) | 0.0% |
| 131 | `add_x_x_x` | `x+x+x` | ![](scripts/snaps/hp/add_x_x_x.png) | ![](scripts/snaps/sdl/add_x_x_x.png) | 0.0% |
| 132 | `add_M_n` | `M+n` | ![](scripts/snaps/hp/add_M_n.png) | ![](scripts/snaps/sdl/add_M_n.png) | 0.0% |
| 133 | `add_u_v` | `u+v` | ![](scripts/snaps/hp/add_u_v.png) | ![](scripts/snaps/sdl/add_u_v.png) | 0.0% |
| 134 | `add_ten_const` | `10+10` | ![](scripts/snaps/hp/add_ten_const.png) | ![](scripts/snaps/sdl/add_ten_const.png) | 0.4% |
| 135 | `add_decim_5_5` | `5.5+5.5` | ![](scripts/snaps/hp/add_decim_5_5.png) | ![](scripts/snaps/sdl/add_decim_5_5.png) | 0.0% |
| 136 | `add_long_var_const` | `VARIABLE+1` | ![](scripts/snaps/hp/add_long_var_const.png) | ![](scripts/snaps/sdl/add_long_var_const.png) | 0.0% |
| 137 | `add_bigname_const` | `ALPHA+1` | ![](scripts/snaps/hp/add_bigname_const.png) | ![](scripts/snaps/sdl/add_bigname_const.png) | 0.0% |
| 138 | `add_bigname_bigname` | `ALPHA+BETA` | ![](scripts/snaps/hp/add_bigname_bigname.png) | ![](scripts/snaps/sdl/add_bigname_bigname.png) | 0.0% |
| 139 | `add_X_neg_3` | `X+-3` | ![](scripts/snaps/hp/add_X_neg_3.png) | ![](scripts/snaps/sdl/add_X_neg_3.png) | 0.7% |
| 140 | `add_neg_X_5` | `-X+5` | ![](scripts/snaps/hp/add_neg_X_5.png) | ![](scripts/snaps/sdl/add_neg_X_5.png) | 0.8% |
| 141 | `add_R_y` | `R+y` | ![](scripts/snaps/hp/add_R_y.png) | ![](scripts/snaps/sdl/add_R_y.png) | 0.0% |

### Subtraction

| # | name | keys | x50ng (HP firmware) | sdleqw | diff |
|---|---|---|---|---|---|
| 142 | `sub_5_3` | `5-3` | ![](scripts/snaps/hp/sub_5_3.png) | ![](scripts/snaps/sdl/sub_5_3.png) | 0.0% |
| 143 | `sub_X_Y` | `X-Y` | ![](scripts/snaps/hp/sub_X_Y.png) | ![](scripts/snaps/sdl/sub_X_Y.png) | 0.0% |
| 144 | `sub_X_5` | `X-5` | ![](scripts/snaps/hp/sub_X_5.png) | ![](scripts/snaps/sdl/sub_X_5.png) | 0.0% |
| 145 | `sub_X_1` | `X-1` | ![](scripts/snaps/hp/sub_X_1.png) | ![](scripts/snaps/sdl/sub_X_1.png) | 0.0% |
| 146 | `sub_a_b` | `A-B` | ![](scripts/snaps/hp/sub_a_b.png) | ![](scripts/snaps/sdl/sub_a_b.png) | 0.0% |
| 147 | `sub_long` | `10-3-2` | ![](scripts/snaps/hp/sub_long.png) | ![](scripts/snaps/sdl/sub_long.png) | 0.6% |
| 148 | `sub_decimal` | `1.5-0.5` | ![](scripts/snaps/hp/sub_decimal.png) | ![](scripts/snaps/sdl/sub_decimal.png) | 0.0% |
| 149 | `sub_negative` | `-1-1` | ![](scripts/snaps/hp/sub_negative.png) | ![](scripts/snaps/sdl/sub_negative.png) | 0.6% |
| 150 | `sub_X_neg_Y` | `X--Y` | ![](scripts/snaps/hp/sub_X_neg_Y.png) | ![](scripts/snaps/sdl/sub_X_neg_Y.png) | 0.5% |
| 151 | `sub_5_X_3` | `5-X-3` | ![](scripts/snaps/hp/sub_5_X_3.png) | ![](scripts/snaps/sdl/sub_5_X_3.png) | 0.5% |
| 152 | `sub_n_1` | `n-1` | ![](scripts/snaps/hp/sub_n_1.png) | ![](scripts/snaps/sdl/sub_n_1.png) | 0.0% |
| 153 | `sub_p_q` | `p-q` | ![](scripts/snaps/hp/sub_p_q.png) | ![](scripts/snaps/sdl/sub_p_q.png) | 0.0% |
| 154 | `sub_R_y` | `R-y` | ![](scripts/snaps/hp/sub_R_y.png) | ![](scripts/snaps/sdl/sub_R_y.png) | 0.0% |
| 155 | `sub_2_X` | `2-X` | ![](scripts/snaps/hp/sub_2_X.png) | ![](scripts/snaps/sdl/sub_2_X.png) | 0.0% |
| 156 | `sub_X_2` | `X-2` | ![](scripts/snaps/hp/sub_X_2.png) | ![](scripts/snaps/sdl/sub_X_2.png) | 0.0% |
| 157 | `sub_PI_3` | `PI-3` | ![](scripts/snaps/hp/sub_PI_3.png) | ![](scripts/snaps/sdl/sub_PI_3.png) | 0.3% |
| 158 | `sub_X_X` | `X-X` | ![](scripts/snaps/hp/sub_X_X.png) | ![](scripts/snaps/sdl/sub_X_X.png) | 0.0% |
| 159 | `sub_a_a_a` | `A-A-A` | ![](scripts/snaps/hp/sub_a_a_a.png) | ![](scripts/snaps/sdl/sub_a_a_a.png) | 0.5% |
| 160 | `sub_x_PI` | `x-PI` | ![](scripts/snaps/hp/sub_x_PI.png) | ![](scripts/snaps/sdl/sub_x_PI.png) | 0.0% |
| 161 | `sub_X_one` | `X-1` | ![](scripts/snaps/hp/sub_X_one.png) | ![](scripts/snaps/sdl/sub_X_one.png) | 0.0% |
| 162 | `sub_X_zero` | `X-0` | ![](scripts/snaps/hp/sub_X_zero.png) | ![](scripts/snaps/sdl/sub_X_zero.png) | 0.0% |
| 163 | `sub_six_minus_two` | `6-2` | ![](scripts/snaps/hp/sub_six_minus_two.png) | ![](scripts/snaps/sdl/sub_six_minus_two.png) | 0.0% |
| 164 | `sub_four_minus_one` | `4-1` | ![](scripts/snaps/hp/sub_four_minus_one.png) | ![](scripts/snaps/sdl/sub_four_minus_one.png) | 0.0% |
| 165 | `sub_seven_minus_three` | `7-3` | ![](scripts/snaps/hp/sub_seven_minus_three.png) | ![](scripts/snaps/sdl/sub_seven_minus_three.png) | 0.0% |
| 166 | `sub_eight_minus_four` | `8-4` | ![](scripts/snaps/hp/sub_eight_minus_four.png) | ![](scripts/snaps/sdl/sub_eight_minus_four.png) | 0.0% |
| 167 | `sub_nine_minus_five` | `9-5` | ![](scripts/snaps/hp/sub_nine_minus_five.png) | ![](scripts/snaps/sdl/sub_nine_minus_five.png) | 0.0% |
| 168 | `sub_Z_R` | `Z-R` | ![](scripts/snaps/hp/sub_Z_R.png) | ![](scripts/snaps/sdl/sub_Z_R.png) | 0.0% |
| 169 | `sub_long_var` | `VARIABLE-1` | ![](scripts/snaps/hp/sub_long_var.png) | ![](scripts/snaps/sdl/sub_long_var.png) | 0.0% |
| 170 | `sub_neg_const` | `-3-X` | ![](scripts/snaps/hp/sub_neg_const.png) | ![](scripts/snaps/sdl/sub_neg_const.png) | 0.8% |

### Multiplication

| # | name | keys | x50ng (HP firmware) | sdleqw | diff |
|---|---|---|---|---|---|
| 171 | `mul_2_3` | `2*3` | ![](scripts/snaps/hp/mul_2_3.png) | ![](scripts/snaps/sdl/mul_2_3.png) | 0.0% |
| 172 | `mul_5_X` | `5*X` | ![](scripts/snaps/hp/mul_5_X.png) | ![](scripts/snaps/sdl/mul_5_X.png) | 0.0% |
| 173 | `mul_X_5` | `X*5` | ![](scripts/snaps/hp/mul_X_5.png) | ![](scripts/snaps/sdl/mul_X_5.png) | 0.0% |
| 174 | `mul_X_Y` | `X*Y` | ![](scripts/snaps/hp/mul_X_Y.png) | ![](scripts/snaps/sdl/mul_X_Y.png) | 0.0% |
| 175 | `mul_a_b_c` | `A*B*R` | ![](scripts/snaps/hp/mul_a_b_c.png) | ![](scripts/snaps/sdl/mul_a_b_c.png) | 0.0% |
| 176 | `mul_2_PI` | `2*PI` | ![](scripts/snaps/hp/mul_2_PI.png) | ![](scripts/snaps/sdl/mul_2_PI.png) | 0.0% |
| 177 | `mul_3_4` | `3*4` | ![](scripts/snaps/hp/mul_3_4.png) | ![](scripts/snaps/sdl/mul_3_4.png) | 0.0% |
| 178 | `mul_decimals` | `1.5*2.5` | ![](scripts/snaps/hp/mul_decimals.png) | ![](scripts/snaps/sdl/mul_decimals.png) | 0.0% |
| 179 | `mul_neg` | `-2*3` | ![](scripts/snaps/hp/mul_neg.png) | ![](scripts/snaps/sdl/mul_neg.png) | 0.6% |
| 180 | `mul_X_2` | `X*2` | ![](scripts/snaps/hp/mul_X_2.png) | ![](scripts/snaps/sdl/mul_X_2.png) | 0.0% |
| 181 | `mul_2_X_3` | `2*X*3` | ![](scripts/snaps/hp/mul_2_X_3.png) | ![](scripts/snaps/sdl/mul_2_X_3.png) | 0.0% |
| 182 | `mul_n_n` | `n*n` | ![](scripts/snaps/hp/mul_n_n.png) | ![](scripts/snaps/sdl/mul_n_n.png) | 0.0% |
| 183 | `mul_X_X_X` | `X*X*X` | ![](scripts/snaps/hp/mul_X_X_X.png) | ![](scripts/snaps/sdl/mul_X_X_X.png) | 0.0% |
| 184 | `mul_3_PI_R` | `3*PI*R` | ![](scripts/snaps/hp/mul_3_PI_R.png) | ![](scripts/snaps/sdl/mul_3_PI_R.png) | 0.5% |
| 185 | `mul_R_t` | `R*t` | ![](scripts/snaps/hp/mul_R_t.png) | ![](scripts/snaps/sdl/mul_R_t.png) | 0.0% |
| 186 | `mul_with_long` | `RHO*V` | ![](scripts/snaps/hp/mul_with_long.png) | ![](scripts/snaps/sdl/mul_with_long.png) | 0.0% |
| 187 | `mul_const_const` | `10*10` | ![](scripts/snaps/hp/mul_const_const.png) | ![](scripts/snaps/sdl/mul_const_const.png) | 0.4% |
| 188 | `mul_lots` | `1*2*3*4` | ![](scripts/snaps/hp/mul_lots.png) | ![](scripts/snaps/sdl/mul_lots.png) | 0.6% |
| 189 | `mul_PI_R_R` | `PI*R*R` | ![](scripts/snaps/hp/mul_PI_R_R.png) | ![](scripts/snaps/sdl/mul_PI_R_R.png) | 0.3% |
| 190 | `mul_decimal_var` | `0.5*X` | ![](scripts/snaps/hp/mul_decimal_var.png) | ![](scripts/snaps/sdl/mul_decimal_var.png) | 0.0% |
| 191 | `mul_2_n` | `2*n` | ![](scripts/snaps/hp/mul_2_n.png) | ![](scripts/snaps/sdl/mul_2_n.png) | 0.0% |
| 192 | `mul_X_neg_1` | `X*-1` | ![](scripts/snaps/hp/mul_X_neg_1.png) | ![](scripts/snaps/sdl/mul_X_neg_1.png) | 0.6% |
| 193 | `mul_neg_X` | `-X*2` | ![](scripts/snaps/hp/mul_neg_X.png) | ![](scripts/snaps/sdl/mul_neg_X.png) | 0.5% |
| 194 | `mul_alpha_beta` | `ALPHA*BETA` | ![](scripts/snaps/hp/mul_alpha_beta.png) | ![](scripts/snaps/sdl/mul_alpha_beta.png) | 0.0% |
| 195 | `mul_xyz` | `x*y*z` | ![](scripts/snaps/hp/mul_xyz.png) | ![](scripts/snaps/sdl/mul_xyz.png) | 0.0% |
| 196 | `mul_pqr` | `p*q*r` | ![](scripts/snaps/hp/mul_pqr.png) | ![](scripts/snaps/sdl/mul_pqr.png) | 0.0% |
| 197 | `mul_2_L` | `2*L` | ![](scripts/snaps/hp/mul_2_L.png) | ![](scripts/snaps/sdl/mul_2_L.png) | 0.0% |
| 198 | `mul_3_OMEGA` | `3*OMEGA` | ![](scripts/snaps/hp/mul_3_OMEGA.png) | ![](scripts/snaps/sdl/mul_3_OMEGA.png) | 0.0% |
| 199 | `mul_THETA_2` | `THETA*2` | ![](scripts/snaps/hp/mul_THETA_2.png) | ![](scripts/snaps/sdl/mul_THETA_2.png) | 0.0% |
| 200 | `mul_zero_X` | `0*X` | ![](scripts/snaps/hp/mul_zero_X.png) | ![](scripts/snaps/sdl/mul_zero_X.png) | 0.0% |

### Division

| # | name | keys | x50ng (HP firmware) | sdleqw | diff |
|---|---|---|---|---|---|
| 201 | `div_1_2` | `1/2` | ![](scripts/snaps/hp/div_1_2.png) | ![](scripts/snaps/sdl/div_1_2.png) | 0.0% |
| 202 | `div_2_3` | `2/3` | ![](scripts/snaps/hp/div_2_3.png) | ![](scripts/snaps/sdl/div_2_3.png) | 0.0% |
| 203 | `div_X_Y` | `X/Y` | ![](scripts/snaps/hp/div_X_Y.png) | ![](scripts/snaps/sdl/div_X_Y.png) | 0.0% |
| 204 | `div_5_X` | `5/X` | ![](scripts/snaps/hp/div_5_X.png) | ![](scripts/snaps/sdl/div_5_X.png) | 0.0% |
| 205 | `div_X_5` | `X/5` | ![](scripts/snaps/hp/div_X_5.png) | ![](scripts/snaps/sdl/div_X_5.png) | 0.0% |
| 206 | `div_X_X` | `X/X` | ![](scripts/snaps/hp/div_X_X.png) | ![](scripts/snaps/sdl/div_X_X.png) | 0.0% |
| 207 | `div_chain` | `1/2/3` | ![](scripts/snaps/hp/div_chain.png) | ![](scripts/snaps/sdl/div_chain.png) | 0.6% |
| 208 | `div_decimals` | `3.14/2` | ![](scripts/snaps/hp/div_decimals.png) | ![](scripts/snaps/sdl/div_decimals.png) | 0.0% |
| 209 | `div_a_b` | `A/B` | ![](scripts/snaps/hp/div_a_b.png) | ![](scripts/snaps/sdl/div_a_b.png) | 0.0% |
| 210 | `div_PI_2` | `PI/2` | ![](scripts/snaps/hp/div_PI_2.png) | ![](scripts/snaps/sdl/div_PI_2.png) | 0.3% |
| 211 | `div_2_PI` | `2/PI` | ![](scripts/snaps/hp/div_2_PI.png) | ![](scripts/snaps/sdl/div_2_PI.png) | 0.0% |
| 212 | `div_X_2` | `X/2` | ![](scripts/snaps/hp/div_X_2.png) | ![](scripts/snaps/sdl/div_X_2.png) | 0.0% |
| 213 | `div_X_neg_2` | `X/-2` | ![](scripts/snaps/hp/div_X_neg_2.png) | ![](scripts/snaps/sdl/div_X_neg_2.png) | 0.3% |
| 214 | `div_neg_X_5` | `-X/5` | ![](scripts/snaps/hp/div_neg_X_5.png) | ![](scripts/snaps/sdl/div_neg_X_5.png) | 0.5% |
| 215 | `div_one_X` | `1/X` | ![](scripts/snaps/hp/div_one_X.png) | ![](scripts/snaps/sdl/div_one_X.png) | 0.0% |
| 216 | `div_X_one` | `X/1` | ![](scripts/snaps/hp/div_X_one.png) | ![](scripts/snaps/sdl/div_X_one.png) | 0.0% |
| 217 | `div_t_5` | `t/5` | ![](scripts/snaps/hp/div_t_5.png) | ![](scripts/snaps/sdl/div_t_5.png) | 0.0% |
| 218 | `div_5_t` | `5/t` | ![](scripts/snaps/hp/div_5_t.png) | ![](scripts/snaps/sdl/div_5_t.png) | 0.0% |
| 219 | `div_R_y` | `R/y` | ![](scripts/snaps/hp/div_R_y.png) | ![](scripts/snaps/sdl/div_R_y.png) | 0.0% |
| 220 | `div_long_var` | `VARIABLE/2` | ![](scripts/snaps/hp/div_long_var.png) | ![](scripts/snaps/sdl/div_long_var.png) | 0.0% |
| 221 | `div_const_var` | `10/X` | ![](scripts/snaps/hp/div_const_var.png) | ![](scripts/snaps/sdl/div_const_var.png) | 0.0% |
| 222 | `div_var_const` | `X/10` | ![](scripts/snaps/hp/div_var_const.png) | ![](scripts/snaps/sdl/div_var_const.png) | 0.4% |
| 223 | `div_two_var` | `X/Y` | ![](scripts/snaps/hp/div_two_var.png) | ![](scripts/snaps/sdl/div_two_var.png) | 0.0% |
| 224 | `div_p_q` | `p/q` | ![](scripts/snaps/hp/div_p_q.png) | ![](scripts/snaps/sdl/div_p_q.png) | 0.0% |
| 225 | `div_n_2` | `n/2` | ![](scripts/snaps/hp/div_n_2.png) | ![](scripts/snaps/sdl/div_n_2.png) | 0.0% |
| 226 | `div_2_n` | `2/n` | ![](scripts/snaps/hp/div_2_n.png) | ![](scripts/snaps/sdl/div_2_n.png) | 0.0% |
| 227 | `div_F_t` | `F/t` | ![](scripts/snaps/hp/div_F_t.png) | ![](scripts/snaps/sdl/div_F_t.png) | 0.0% |
| 228 | `div_a_b_c` | `A/B/R` | ![](scripts/snaps/hp/div_a_b_c.png) | ![](scripts/snaps/sdl/div_a_b_c.png) | 0.7% |
| 229 | `div_xyz` | `x/y/z` | ![](scripts/snaps/hp/div_xyz.png) | ![](scripts/snaps/sdl/div_xyz.png) | 0.5% |
| 230 | `div_neg_a_b` | `-A/B` | ![](scripts/snaps/hp/div_neg_a_b.png) | ![](scripts/snaps/sdl/div_neg_a_b.png) | 0.5% |

### Powers

| # | name | keys | x50ng (HP firmware) | sdleqw | diff |
|---|---|---|---|---|---|
| 231 | `pow_X_2` | `X^2` | ![](scripts/snaps/hp/pow_X_2.png) | ![](scripts/snaps/sdl/pow_X_2.png) | 0.0% |
| 232 | `pow_X_3` | `X^3` | ![](scripts/snaps/hp/pow_X_3.png) | ![](scripts/snaps/sdl/pow_X_3.png) | 0.0% |
| 233 | `pow_2_X` | `2^X` | ![](scripts/snaps/hp/pow_2_X.png) | ![](scripts/snaps/sdl/pow_2_X.png) | 0.0% |
| 234 | `pow_X_n` | `X^n` | ![](scripts/snaps/hp/pow_X_n.png) | ![](scripts/snaps/sdl/pow_X_n.png) | 0.0% |
| 235 | `pow_X_Y` | `X^Y` | ![](scripts/snaps/hp/pow_X_Y.png) | ![](scripts/snaps/sdl/pow_X_Y.png) | 0.0% |
| 236 | `pow_e_X` | `e^X` | ![](scripts/snaps/hp/pow_e_X.png) | ![](scripts/snaps/sdl/pow_e_X.png) | 0.0% |
| 237 | `pow_X_2_plus_1` | `X^2+1` | ![](scripts/snaps/hp/pow_X_2_plus_1.png) | ![](scripts/snaps/sdl/pow_X_2_plus_1.png) | 1.0% |
| 238 | `pow_pow` | `X^Y^Z` | ![](scripts/snaps/hp/pow_pow.png) | ![](scripts/snaps/sdl/pow_pow.png) | 0.0% |
| 239 | `pow_decimal` | `X^0.5` | ![](scripts/snaps/hp/pow_decimal.png) | ![](scripts/snaps/sdl/pow_decimal.png) | 0.0% |
| 240 | `pow_neg` | `X^-1` | ![](scripts/snaps/hp/pow_neg.png) | ![](scripts/snaps/sdl/pow_neg.png) | 0.6% |
| 241 | `pow_N_2` | `N^2` | ![](scripts/snaps/hp/pow_N_2.png) | ![](scripts/snaps/sdl/pow_N_2.png) | 0.0% |
| 242 | `pow_R_2` | `R^2` | ![](scripts/snaps/hp/pow_R_2.png) | ![](scripts/snaps/sdl/pow_R_2.png) | 0.0% |
| 243 | `pow_t_2` | `t^2` | ![](scripts/snaps/hp/pow_t_2.png) | ![](scripts/snaps/sdl/pow_t_2.png) | 0.0% |
| 244 | `pow_y_3` | `y^3` | ![](scripts/snaps/hp/pow_y_3.png) | ![](scripts/snaps/sdl/pow_y_3.png) | 0.0% |
| 245 | `pow_a_b` | `A^B` | ![](scripts/snaps/hp/pow_a_b.png) | ![](scripts/snaps/sdl/pow_a_b.png) | 0.0% |
| 246 | `pow_PI_X_2` | `PI*X^2` | ![](scripts/snaps/hp/pow_PI_X_2.png) | ![](scripts/snaps/sdl/pow_PI_X_2.png) | 0.3% |
| 247 | `pow_2_2_2` | `2^2^2` | ![](scripts/snaps/hp/pow_2_2_2.png) | ![](scripts/snaps/sdl/pow_2_2_2.png) | 0.0% |
| 248 | `pow_X_4` | `X^4` | ![](scripts/snaps/hp/pow_X_4.png) | ![](scripts/snaps/sdl/pow_X_4.png) | 0.0% |
| 249 | `pow_X_5` | `X^5` | ![](scripts/snaps/hp/pow_X_5.png) | ![](scripts/snaps/sdl/pow_X_5.png) | 0.0% |
| 250 | `pow_X_6` | `X^6` | ![](scripts/snaps/hp/pow_X_6.png) | ![](scripts/snaps/sdl/pow_X_6.png) | 0.0% |
| 251 | `pow_X_10` | `X^10` | ![](scripts/snaps/hp/pow_X_10.png) | ![](scripts/snaps/sdl/pow_X_10.png) | 0.2% |
| 252 | `pow_X_neg_2` | `X^-2` | ![](scripts/snaps/hp/pow_X_neg_2.png) | ![](scripts/snaps/sdl/pow_X_neg_2.png) | 0.7% |
| 253 | `pow_X_X` | `X^X` | ![](scripts/snaps/hp/pow_X_X.png) | ![](scripts/snaps/sdl/pow_X_X.png) | 0.0% |
| 254 | `pow_n_k` | `n^k` | ![](scripts/snaps/hp/pow_n_k.png) | ![](scripts/snaps/sdl/pow_n_k.png) | 0.0% |
| 255 | `pow_x_third` | `X^0.33` | ![](scripts/snaps/hp/pow_x_third.png) | ![](scripts/snaps/sdl/pow_x_third.png) | 1.2% |
| 256 | `pow_2_n_minus_1` | `2^n-1` | ![](scripts/snaps/hp/pow_2_n_minus_1.png) | ![](scripts/snaps/sdl/pow_2_n_minus_1.png) | 1.1% |
| 257 | `pow_n_n` | `n^n` | ![](scripts/snaps/hp/pow_n_n.png) | ![](scripts/snaps/sdl/pow_n_n.png) | 0.0% |
| 258 | `pow_OMEGA_X` | `OMEGA*X` | ![](scripts/snaps/hp/pow_OMEGA_X.png) | ![](scripts/snaps/sdl/pow_OMEGA_X.png) | 0.0% |
| 259 | `pow_a_inv` | `a^-1` | ![](scripts/snaps/hp/pow_a_inv.png) | ![](scripts/snaps/sdl/pow_a_inv.png) | 0.6% |
| 260 | `pow_var_2` | `VARIABLE^2` | ![](scripts/snaps/hp/pow_var_2.png) | ![](scripts/snaps/sdl/pow_var_2.png) | 0.0% |

### Square roots

| # | name | keys | x50ng (HP firmware) | sdleqw | diff |
|---|---|---|---|---|---|
| 261 | `sqrt_just` | `@` | ![](scripts/snaps/hp/sqrt_just.png) | ![](scripts/snaps/sdl/sqrt_just.png) | 0.4% |
| 262 | `sqrt_X_after` | `X@` | ![](scripts/snaps/hp/sqrt_X_after.png) | ![](scripts/snaps/sdl/sqrt_X_after.png) | 1.1% |
| 263 | `sqrt_then_X` | `@X` | ![](scripts/snaps/hp/sqrt_then_X.png) | ![](scripts/snaps/sdl/sqrt_then_X.png) | 0.4% |
| 264 | `sqrt_2_after` | `2@` | ![](scripts/snaps/hp/sqrt_2_after.png) | ![](scripts/snaps/sdl/sqrt_2_after.png) | 1.1% |
| 265 | `sqrt_3_after` | `3@` | ![](scripts/snaps/hp/sqrt_3_after.png) | ![](scripts/snaps/sdl/sqrt_3_after.png) | 1.2% |
| 266 | `sqrt_then_2` | `@2` | ![](scripts/snaps/hp/sqrt_then_2.png) | ![](scripts/snaps/sdl/sqrt_then_2.png) | 0.4% |
| 267 | `sqrt_then_4` | `@4` | ![](scripts/snaps/hp/sqrt_then_4.png) | ![](scripts/snaps/sdl/sqrt_then_4.png) | 0.4% |
| 268 | `sqrt_X_squared` | `@X^2` | ![](scripts/snaps/hp/sqrt_X_squared.png) | ![](scripts/snaps/sdl/sqrt_X_squared.png) | 0.7% |
| 269 | `sqrt_then_sum` | `@1+1` | ![](scripts/snaps/hp/sqrt_then_sum.png) | ![](scripts/snaps/sdl/sqrt_then_sum.png) | 0.7% |
| 270 | `sqrt_X_plus_Y` | `@X+Y` | ![](scripts/snaps/hp/sqrt_X_plus_Y.png) | ![](scripts/snaps/sdl/sqrt_X_plus_Y.png) | 0.7% |
| 271 | `sqrt_a_div_b` | `@A/B` | ![](scripts/snaps/hp/sqrt_a_div_b.png) | ![](scripts/snaps/sdl/sqrt_a_div_b.png) | 1.2% |
| 272 | `sqrt_X_plus_1` | `@X+1` | ![](scripts/snaps/hp/sqrt_X_plus_1.png) | ![](scripts/snaps/sdl/sqrt_X_plus_1.png) | 0.7% |
| 273 | `sqrt_PI` | `@PI` | ![](scripts/snaps/hp/sqrt_PI.png) | ![](scripts/snaps/sdl/sqrt_PI.png) | 0.5% |
| 274 | `sqrt_long` | `@VARIABLE` | ![](scripts/snaps/hp/sqrt_long.png) | ![](scripts/snaps/sdl/sqrt_long.png) | 1.6% |
| 275 | `sqrt_in_div` | `@4/2` | ![](scripts/snaps/hp/sqrt_in_div.png) | ![](scripts/snaps/sdl/sqrt_in_div.png) | 1.0% |
| 276 | `sqrt_R` | `@R` | ![](scripts/snaps/hp/sqrt_R.png) | ![](scripts/snaps/sdl/sqrt_R.png) | 0.4% |
| 277 | `sqrt_t` | `@t` | ![](scripts/snaps/hp/sqrt_t.png) | ![](scripts/snaps/sdl/sqrt_t.png) | 0.4% |
| 278 | `sqrt_after_n` | `@n` | ![](scripts/snaps/hp/sqrt_after_n.png) | ![](scripts/snaps/sdl/sqrt_after_n.png) | 0.4% |
| 279 | `sqrt_after_long_name` | `@ALPHA` | ![](scripts/snaps/hp/sqrt_after_long_name.png) | ![](scripts/snaps/sdl/sqrt_after_long_name.png) | 1.0% |
| 280 | `sqrt_after_minus` | `-@4` | ![](scripts/snaps/hp/sqrt_after_minus.png) | ![](scripts/snaps/sdl/sqrt_after_minus.png) | 0.5% |

### Nth roots

| # | name | keys | x50ng (HP firmware) | sdleqw | diff |
|---|---|---|---|---|---|
| 281 | `nthroot_3_X` | `#3$rX` | ![](scripts/snaps/hp/nthroot_3_X.png) | ![](scripts/snaps/sdl/nthroot_3_X.png) | 0.4% |
| 282 | `nthroot_2_4` | `#2$r4` | ![](scripts/snaps/hp/nthroot_2_4.png) | ![](scripts/snaps/sdl/nthroot_2_4.png) | 0.5% |
| 283 | `nthroot_3_8` | `#3$r8` | ![](scripts/snaps/hp/nthroot_3_8.png) | ![](scripts/snaps/sdl/nthroot_3_8.png) | 0.5% |
| 284 | `nthroot_4_16` | `#4$r16` | ![](scripts/snaps/hp/nthroot_4_16.png) | ![](scripts/snaps/sdl/nthroot_4_16.png) | 0.7% |
| 285 | `nthroot_5_X` | `#5$rX` | ![](scripts/snaps/hp/nthroot_5_X.png) | ![](scripts/snaps/sdl/nthroot_5_X.png) | 0.4% |
| 286 | `nthroot_n_X` | `#n$rX` | ![](scripts/snaps/hp/nthroot_n_X.png) | ![](scripts/snaps/sdl/nthroot_n_X.png) | 0.4% |
| 287 | `nthroot_3_a` | `#3$ra` | ![](scripts/snaps/hp/nthroot_3_a.png) | ![](scripts/snaps/sdl/nthroot_3_a.png) | 0.5% |
| 288 | `nthroot_X_2` | `#X$r2` | ![](scripts/snaps/hp/nthroot_X_2.png) | ![](scripts/snaps/sdl/nthroot_X_2.png) | 0.5% |
| 289 | `nthroot_2_X_squared` | `#2$rX^2` | ![](scripts/snaps/hp/nthroot_2_X_squared.png) | ![](scripts/snaps/sdl/nthroot_2_X_squared.png) | 0.8% |
| 290 | `nthroot_n_2` | `#n$r2` | ![](scripts/snaps/hp/nthroot_n_2.png) | ![](scripts/snaps/sdl/nthroot_n_2.png) | 0.5% |

### Parens

| # | name | keys | x50ng (HP firmware) | sdleqw | diff |
|---|---|---|---|---|---|
| 291 | `paren_around_X` | `X(` | ![](scripts/snaps/hp/paren_around_X.png) | ![](scripts/snaps/sdl/paren_around_X.png) | 1.0% |
| 292 | `paren_around_5` | `5(` | ![](scripts/snaps/hp/paren_around_5.png) | ![](scripts/snaps/sdl/paren_around_5.png) | 1.0% |
| 293 | `paren_around_a_plus_b` | `A+B(` | ![](scripts/snaps/hp/paren_around_a_plus_b.png) | ![](scripts/snaps/sdl/paren_around_a_plus_b.png) | 1.3% |
| 294 | `paren_around_X_div_Y` | `X/Y(` | ![](scripts/snaps/hp/paren_around_X_div_Y.png) | ![](scripts/snaps/sdl/paren_around_X_div_Y.png) | 1.6% |
| 295 | `paren_double` | `X((` | ![](scripts/snaps/hp/paren_double.png) | ![](scripts/snaps/sdl/paren_double.png) | 1.2% |
| 296 | `paren_then_op` | `X(+1` | ![](scripts/snaps/hp/paren_then_op.png) | ![](scripts/snaps/sdl/paren_then_op.png) | 0.7% |
| 297 | `paren_around_sum` | `X+Y(` | ![](scripts/snaps/hp/paren_around_sum.png) | ![](scripts/snaps/sdl/paren_around_sum.png) | 1.3% |
| 298 | `paren_around_diff` | `X-Y(` | ![](scripts/snaps/hp/paren_around_diff.png) | ![](scripts/snaps/sdl/paren_around_diff.png) | 1.2% |
| 299 | `paren_around_pow` | `X^2(` | ![](scripts/snaps/hp/paren_around_pow.png) | ![](scripts/snaps/sdl/paren_around_pow.png) | 1.3% |
| 300 | `paren_around_decimal` | `3.14(` | ![](scripts/snaps/hp/paren_around_decimal.png) | ![](scripts/snaps/sdl/paren_around_decimal.png) | 1.7% |
| 301 | `paren_around_neg_X` | `-X(` | ![](scripts/snaps/hp/paren_around_neg_X.png) | ![](scripts/snaps/sdl/paren_around_neg_X.png) | 0.8% |
| 302 | `paren_X_div_2` | `X(/2` | ![](scripts/snaps/hp/paren_X_div_2.png) | ![](scripts/snaps/sdl/paren_X_div_2.png) | 0.9% |
| 303 | `paren_around_abc` | `ABC(` | ![](scripts/snaps/hp/paren_around_abc.png) | ![](scripts/snaps/sdl/paren_around_abc.png) | 1.6% |
| 304 | `paren_around_PI` | `PI(` | ![](scripts/snaps/hp/paren_around_PI.png) | ![](scripts/snaps/sdl/paren_around_PI.png) | 1.2% |
| 305 | `paren_around_long` | `VARIABLE(` | ![](scripts/snaps/hp/paren_around_long.png) | ![](scripts/snaps/sdl/paren_around_long.png) | 3.2% |

### Absolute value

| # | name | keys | x50ng (HP firmware) | sdleqw | diff |
|---|---|---|---|---|---|
| 306 | `abs_X` | `X|` | ![](scripts/snaps/hp/abs_X.png) | ![](scripts/snaps/sdl/abs_X.png) | 1.0% |
| 307 | `abs_5` | `5|` | ![](scripts/snaps/hp/abs_5.png) | ![](scripts/snaps/sdl/abs_5.png) | 0.9% |
| 308 | `abs_X_plus_Y` | `X+Y|` | ![](scripts/snaps/hp/abs_X_plus_Y.png) | ![](scripts/snaps/sdl/abs_X_plus_Y.png) | 1.2% |
| 309 | `abs_X_minus_5` | `X-5|` | ![](scripts/snaps/hp/abs_X_minus_5.png) | ![](scripts/snaps/sdl/abs_X_minus_5.png) | 1.2% |
| 310 | `abs_X_squared` | `X^2|` | ![](scripts/snaps/hp/abs_X_squared.png) | ![](scripts/snaps/sdl/abs_X_squared.png) | 1.4% |
| 311 | `abs_X_div_Y` | `X/Y|` | ![](scripts/snaps/hp/abs_X_div_Y.png) | ![](scripts/snaps/sdl/abs_X_div_Y.png) | 1.8% |
| 312 | `abs_PI` | `PI|` | ![](scripts/snaps/hp/abs_PI.png) | ![](scripts/snaps/sdl/abs_PI.png) | 1.3% |
| 313 | `abs_neg_X` | `-X|` | ![](scripts/snaps/hp/abs_neg_X.png) | ![](scripts/snaps/sdl/abs_neg_X.png) | 1.2% |
| 314 | `abs_long_name` | `VARIABLE|` | ![](scripts/snaps/hp/abs_long_name.png) | ![](scripts/snaps/sdl/abs_long_name.png) | 2.9% |
| 315 | `abs_a_plus_b` | `A+B|` | ![](scripts/snaps/hp/abs_a_plus_b.png) | ![](scripts/snaps/sdl/abs_a_plus_b.png) | 1.2% |

### Prefix functions

| # | name | keys | x50ng (HP firmware) | sdleqw | diff |
|---|---|---|---|---|---|
| 316 | `sin_X` | `X$s` | ![](scripts/snaps/hp/sin_X.png) | ![](scripts/snaps/sdl/sin_X.png) | 1.2% |
| 317 | `sin_2` | `2$s` | ![](scripts/snaps/hp/sin_2.png) | ![](scripts/snaps/sdl/sin_2.png) | 1.2% |
| 318 | `sin_PI` | `PI$s` | ![](scripts/snaps/hp/sin_PI.png) | ![](scripts/snaps/sdl/sin_PI.png) | 1.1% |
| 319 | `cos_X` | `X$c` | ![](scripts/snaps/hp/cos_X.png) | ![](scripts/snaps/sdl/cos_X.png) | 1.4% |
| 320 | `cos_2` | `2$c` | ![](scripts/snaps/hp/cos_2.png) | ![](scripts/snaps/sdl/cos_2.png) | 1.4% |
| 321 | `tan_X` | `X$T` | ![](scripts/snaps/hp/tan_X.png) | ![](scripts/snaps/sdl/tan_X.png) | 1.2% |
| 322 | `ln_X` | `X$n` | ![](scripts/snaps/hp/ln_X.png) | ![](scripts/snaps/sdl/ln_X.png) | 1.0% |
| 323 | `ln_e` | `e$n` | ![](scripts/snaps/hp/ln_e.png) | ![](scripts/snaps/sdl/ln_e.png) | 1.0% |
| 324 | `exp_X` | `X$X` | ![](scripts/snaps/hp/exp_X.png) | ![](scripts/snaps/sdl/exp_X.png) | 1.0% |
| 325 | `exp_neg_X` | `-X$X` | ![](scripts/snaps/hp/exp_neg_X.png) | ![](scripts/snaps/sdl/exp_neg_X.png) | 1.0% |
| 326 | `log_X` | `X$g` | ![](scripts/snaps/hp/log_X.png) | ![](scripts/snaps/sdl/log_X.png) | 1.3% |
| 327 | `log_10` | `10$g` | ![](scripts/snaps/hp/log_10.png) | ![](scripts/snaps/sdl/log_10.png) | 1.1% |

### Other

| # | name | keys | x50ng (HP firmware) | sdleqw | diff |
|---|---|---|---|---|---|
| 328 | `absfn_X` | `X$a` | ![](scripts/snaps/hp/absfn_X.png) | ![](scripts/snaps/sdl/absfn_X.png) | 1.0% |

### Prefix functions

| # | name | keys | x50ng (HP firmware) | sdleqw | diff |
|---|---|---|---|---|---|
| 329 | `sin_X_plus_Y` | `X+Y$s` | ![](scripts/snaps/hp/sin_X_plus_Y.png) | ![](scripts/snaps/sdl/sin_X_plus_Y.png) | 1.6% |
| 330 | `cos_X_plus_Y` | `X+Y$c` | ![](scripts/snaps/hp/cos_X_plus_Y.png) | ![](scripts/snaps/sdl/cos_X_plus_Y.png) | 1.7% |
| 331 | `sin_X_div_2` | `X/2$s` | ![](scripts/snaps/hp/sin_X_div_2.png) | ![](scripts/snaps/sdl/sin_X_div_2.png) | 2.3% |
| 332 | `cos_X_2` | `X^2$c` | ![](scripts/snaps/hp/cos_X_2.png) | ![](scripts/snaps/sdl/cos_X_2.png) | 1.8% |
| 333 | `ln_X_plus_1` | `X+1$n` | ![](scripts/snaps/hp/ln_X_plus_1.png) | ![](scripts/snaps/sdl/ln_X_plus_1.png) | 1.2% |
| 334 | `sin_5` | `5$s` | ![](scripts/snaps/hp/sin_5.png) | ![](scripts/snaps/sdl/sin_5.png) | 1.3% |
| 335 | `sin_neg_X` | `-X$s` | ![](scripts/snaps/hp/sin_neg_X.png) | ![](scripts/snaps/sdl/sin_neg_X.png) | 0.2% |
| 336 | `ln_2` | `2$n` | ![](scripts/snaps/hp/ln_2.png) | ![](scripts/snaps/sdl/ln_2.png) | 1.0% |
| 337 | `exp_2X` | `2*X$X` | ![](scripts/snaps/hp/exp_2X.png) | ![](scripts/snaps/sdl/exp_2X.png) | 1.2% |
| 338 | `sin_X_squared` | `X^2$s` | ![](scripts/snaps/hp/sin_X_squared.png) | ![](scripts/snaps/sdl/sin_X_squared.png) | 1.7% |
| 339 | `cos_X_squared` | `X^2$c` | ![](scripts/snaps/hp/cos_X_squared.png) | ![](scripts/snaps/sdl/cos_X_squared.png) | 1.8% |
| 340 | `tan_X_plus_PI` | `X+PI$T` | ![](scripts/snaps/hp/tan_X_plus_PI.png) | ![](scripts/snaps/sdl/tan_X_plus_PI.png) | 1.5% |
| 341 | `log_PI` | `PI$g` | ![](scripts/snaps/hp/log_PI.png) | ![](scripts/snaps/sdl/log_PI.png) | 1.3% |
| 342 | `ln_PI` | `PI$n` | ![](scripts/snaps/hp/ln_PI.png) | ![](scripts/snaps/sdl/ln_PI.png) | 1.1% |
| 343 | `exp_pi_x` | `PI*X$X` | ![](scripts/snaps/hp/exp_pi_x.png) | ![](scripts/snaps/sdl/exp_pi_x.png) | 1.3% |
| 344 | `sin_PI_2` | `PI/2$s` | ![](scripts/snaps/hp/sin_PI_2.png) | ![](scripts/snaps/sdl/sin_PI_2.png) | 2.3% |
| 345 | `cos_PI_2` | `PI/2$c` | ![](scripts/snaps/hp/cos_PI_2.png) | ![](scripts/snaps/sdl/cos_PI_2.png) | 2.3% |
| 346 | `sin_X_minus_1` | `X-1$s` | ![](scripts/snaps/hp/sin_X_minus_1.png) | ![](scripts/snaps/sdl/sin_X_minus_1.png) | 1.1% |
| 347 | `ln_VARIABLE` | `VARIABLE$n` | ![](scripts/snaps/hp/ln_VARIABLE.png) | ![](scripts/snaps/sdl/ln_VARIABLE.png) | 2.5% |
| 348 | `sin_then_paren_X` | `X$s(` | ![](scripts/snaps/hp/sin_then_paren_X.png) | ![](scripts/snaps/sdl/sin_then_paren_X.png) | 1.4% |
| 349 | `sin_X_times_2` | `X*2$s` | ![](scripts/snaps/hp/sin_X_times_2.png) | ![](scripts/snaps/sdl/sin_X_times_2.png) | 1.5% |
| 350 | `ln_X_X` | `X*X$n` | ![](scripts/snaps/hp/ln_X_X.png) | ![](scripts/snaps/sdl/ln_X_X.png) | 1.2% |

### Integrals

| # | name | keys | x50ng (HP firmware) | sdleqw | diff |
|---|---|---|---|---|---|
| 351 | `integ_basic` | `$I0$rX$rX$rX` | ![](scripts/snaps/hp/integ_basic.png) | ![](scripts/snaps/sdl/integ_basic.png) | 1.4% |
| 352 | `integ_0_to_1` | `$I0$r1$rX$rX` | ![](scripts/snaps/hp/integ_0_to_1.png) | ![](scripts/snaps/sdl/integ_0_to_1.png) | 1.4% |
| 353 | `integ_a_to_b` | `$Ia$rb$rX$rX` | ![](scripts/snaps/hp/integ_a_to_b.png) | ![](scripts/snaps/sdl/integ_a_to_b.png) | 1.5% |
| 354 | `integ_0_to_inf` | `$I0$rINF$rX$rX` | ![](scripts/snaps/hp/integ_0_to_inf.png) | ![](scripts/snaps/sdl/integ_0_to_inf.png) | 1.4% |
| 355 | `integ_doc` | `$I0$r1/X$rABS$r$rt` | ![](scripts/snaps/hp/integ_doc.png) | ![](scripts/snaps/sdl/integ_doc.png) | 1.9% |
| 356 | `integ_X2` | `$I0$r1$rX^2$rX` | ![](scripts/snaps/hp/integ_X2.png) | ![](scripts/snaps/sdl/integ_X2.png) | 1.5% |
| 357 | `integ_sin` | `$I0$rPI$rX$s$rX` | ![](scripts/snaps/hp/integ_sin.png) | ![](scripts/snaps/sdl/integ_sin.png) | 2.3% |
| 358 | `integ_cos` | `$I0$rPI$rX$c$rX` | ![](scripts/snaps/hp/integ_cos.png) | ![](scripts/snaps/sdl/integ_cos.png) | 2.3% |
| 359 | `integ_simple_2` | `$I1$r2$rX$rX` | ![](scripts/snaps/hp/integ_simple_2.png) | ![](scripts/snaps/sdl/integ_simple_2.png) | 1.4% |
| 360 | `integ_simple_3` | `$I0$r10$rX$rX` | ![](scripts/snaps/hp/integ_simple_3.png) | ![](scripts/snaps/sdl/integ_simple_3.png) | 1.2% |
| 361 | `integ_sub` | `$I0$r1$r2*X$rX` | ![](scripts/snaps/hp/integ_sub.png) | ![](scripts/snaps/sdl/integ_sub.png) | 1.5% |
| 362 | `integ_div` | `$I1$r2$r1/X$rX` | ![](scripts/snaps/hp/integ_div.png) | ![](scripts/snaps/sdl/integ_div.png) | 1.5% |
| 363 | `integ_pow` | `$I0$r1$rX^2$rX` | ![](scripts/snaps/hp/integ_pow.png) | ![](scripts/snaps/sdl/integ_pow.png) | 1.5% |
| 364 | `integ_sqrt` | `$I0$r1$r@X$rX` | ![](scripts/snaps/hp/integ_sqrt.png) | ![](scripts/snaps/sdl/integ_sqrt.png) | 1.5% |
| 365 | `integ_abs` | `$I0$r1$rX|$rX` | ![](scripts/snaps/hp/integ_abs.png) | ![](scripts/snaps/sdl/integ_abs.png) | 2.1% |
| 366 | `integ_basic_t` | `$I0$rT$rX$rT` | ![](scripts/snaps/hp/integ_basic_t.png) | ![](scripts/snaps/sdl/integ_basic_t.png) | 1.3% |
| 367 | `integ_constant` | `$I0$r1$r1$rX` | ![](scripts/snaps/hp/integ_constant.png) | ![](scripts/snaps/sdl/integ_constant.png) | 0.9% |
| 368 | `integ_zero` | `$I0$r0$rX$rX` | ![](scripts/snaps/hp/integ_zero.png) | ![](scripts/snaps/sdl/integ_zero.png) | 1.5% |
| 369 | `integ_neg` | `$I0$r1$r-X$rX` | ![](scripts/snaps/hp/integ_neg.png) | ![](scripts/snaps/sdl/integ_neg.png) | 1.3% |
| 370 | `integ_sum_inside` | `$I0$r1$rX+Y$rX` | ![](scripts/snaps/hp/integ_sum_inside.png) | ![](scripts/snaps/sdl/integ_sum_inside.png) | 1.5% |
| 371 | `integ_long_name` | `$Ia$rb$rABS$rt` | ![](scripts/snaps/hp/integ_long_name.png) | ![](scripts/snaps/sdl/integ_long_name.png) | 1.8% |
| 372 | `integ_lo_X_hi_2X` | `$IX$r2*X$rX$rX` | ![](scripts/snaps/hp/integ_lo_X_hi_2X.png) | ![](scripts/snaps/sdl/integ_lo_X_hi_2X.png) | 1.5% |
| 373 | `integ_F_of_t` | `$I0$r1$rF$rt` | ![](scripts/snaps/hp/integ_F_of_t.png) | ![](scripts/snaps/sdl/integ_F_of_t.png) | 1.2% |
| 374 | `integ_lo_PI_hi_2PI` | `$IPI$r2*PI$rX$rX` | ![](scripts/snaps/hp/integ_lo_PI_hi_2PI.png) | ![](scripts/snaps/sdl/integ_lo_PI_hi_2PI.png) | 1.8% |
| 375 | `integ_X_2` | `$I0$rX$r2$rX` | ![](scripts/snaps/hp/integ_X_2.png) | ![](scripts/snaps/sdl/integ_X_2.png) | 1.4% |

### Summations

| # | name | keys | x50ng (HP firmware) | sdleqw | diff |
|---|---|---|---|---|---|
| 376 | `sum_basic` | `$Sk$r1$rn$rk` | ![](scripts/snaps/hp/sum_basic.png) | ![](scripts/snaps/sdl/sum_basic.png) | 1.4% |
| 377 | `sum_squared` | `$Sk$r1$rn$rk^2` | ![](scripts/snaps/hp/sum_squared.png) | ![](scripts/snaps/sdl/sum_squared.png) | 2.0% |
| 378 | `sum_cubed` | `$Sk$r1$rn$rk^3` | ![](scripts/snaps/hp/sum_cubed.png) | ![](scripts/snaps/sdl/sum_cubed.png) | 2.0% |
| 379 | `sum_geom` | `$Sk$r0$rn$rX^k` | ![](scripts/snaps/hp/sum_geom.png) | ![](scripts/snaps/sdl/sum_geom.png) | 2.0% |
| 380 | `sum_long` | `$Si$r1$rN$ri` | ![](scripts/snaps/hp/sum_long.png) | ![](scripts/snaps/sdl/sum_long.png) | 1.2% |
| 381 | `sum_const` | `$Sk$r1$rn$r1` | ![](scripts/snaps/hp/sum_const.png) | ![](scripts/snaps/sdl/sum_const.png) | 1.3% |
| 382 | `sum_to_inf` | `$Sk$r0$rINF$rk` | ![](scripts/snaps/hp/sum_to_inf.png) | ![](scripts/snaps/sdl/sum_to_inf.png) | 1.6% |
| 383 | `sum_div` | `$Sk$r1$rn$r1/k` | ![](scripts/snaps/hp/sum_div.png) | ![](scripts/snaps/sdl/sum_div.png) | 2.2% |
| 384 | `sum_X_k` | `$Sk$r1$rn$rX*k` | ![](scripts/snaps/hp/sum_X_k.png) | ![](scripts/snaps/sdl/sum_X_k.png) | 1.6% |
| 385 | `sum_negative` | `$Sk$r1$rn$r-k` | ![](scripts/snaps/hp/sum_negative.png) | ![](scripts/snaps/sdl/sum_negative.png) | 1.0% |
| 386 | `sum_inv_k` | `$Si$r1$rn$r1/i` | ![](scripts/snaps/hp/sum_inv_k.png) | ![](scripts/snaps/sdl/sum_inv_k.png) | 2.1% |
| 387 | `sum_basic_two` | `$Sk$r2$r5$rk` | ![](scripts/snaps/hp/sum_basic_two.png) | ![](scripts/snaps/sdl/sum_basic_two.png) | 1.4% |
| 388 | `sum_func` | `$Sk$r1$rn$rk$s` | ![](scripts/snaps/hp/sum_func.png) | ![](scripts/snaps/sdl/sum_func.png) | 1.9% |
| 389 | `sum_polynomial` | `$Sk$r0$rn$rk^2+1` | ![](scripts/snaps/hp/sum_polynomial.png) | ![](scripts/snaps/sdl/sum_polynomial.png) | 2.8% |
| 390 | `sum_double` | `$Sk$r1$rn$rk*X` | ![](scripts/snaps/hp/sum_double.png) | ![](scripts/snaps/sdl/sum_double.png) | 1.6% |

### Derivatives

| # | name | keys | x50ng (HP firmware) | sdleqw | diff |
|---|---|---|---|---|---|
| 391 | `deriv_X_X` | `$PX$rX` | ![](scripts/snaps/hp/deriv_X_X.png) | ![](scripts/snaps/sdl/deriv_X_X.png) | 1.0% |
| 392 | `deriv_X_X2` | `$PX$rX^2` | ![](scripts/snaps/hp/deriv_X_X2.png) | ![](scripts/snaps/sdl/deriv_X_X2.png) | 1.6% |
| 393 | `deriv_X_sin_X` | `$PX$rX$s` | ![](scripts/snaps/hp/deriv_X_sin_X.png) | ![](scripts/snaps/sdl/deriv_X_sin_X.png) | 1.8% |
| 394 | `deriv_t_X_squared` | `$Pt$rX^2` | ![](scripts/snaps/hp/deriv_t_X_squared.png) | ![](scripts/snaps/sdl/deriv_t_X_squared.png) | 1.6% |
| 395 | `deriv_X_X_div_X` | `$PX$rX/X` | ![](scripts/snaps/hp/deriv_X_X_div_X.png) | ![](scripts/snaps/sdl/deriv_X_X_div_X.png) | 1.8% |
| 396 | `deriv_X_e_x` | `$PX$re^X` | ![](scripts/snaps/hp/deriv_X_e_x.png) | ![](scripts/snaps/sdl/deriv_X_e_x.png) | 1.6% |
| 397 | `deriv_X_const` | `$PX$r1` | ![](scripts/snaps/hp/deriv_X_const.png) | ![](scripts/snaps/sdl/deriv_X_const.png) | 0.8% |
| 398 | `deriv_X_PI` | `$PX$rPI` | ![](scripts/snaps/hp/deriv_X_PI.png) | ![](scripts/snaps/sdl/deriv_X_PI.png) | 1.0% |
| 399 | `deriv_X_X_plus_Y` | `$PX$rX+Y` | ![](scripts/snaps/hp/deriv_X_X_plus_Y.png) | ![](scripts/snaps/sdl/deriv_X_X_plus_Y.png) | 1.0% |
| 400 | `deriv_X_X_squared_plus_1` | `$PX$rX^2+1` | ![](scripts/snaps/hp/deriv_X_X_squared_plus_1.png) | ![](scripts/snaps/sdl/deriv_X_X_squared_plus_1.png) | 2.1% |

### Where

| # | name | keys | x50ng (HP firmware) | sdleqw | diff |
|---|---|---|---|---|---|
| 401 | `where_X_X_5` | `X+1$WX$r5` | ![](scripts/snaps/hp/where_X_X_5.png) | ![](scripts/snaps/sdl/where_X_X_5.png) | 0.8% |
| 402 | `where_X2_X_2` | `X^2$WX$r2` | ![](scripts/snaps/hp/where_X2_X_2.png) | ![](scripts/snaps/sdl/where_X2_X_2.png) | 0.8% |
| 403 | `where_X_PI` | `X*2$WX$rPI` | ![](scripts/snaps/hp/where_X_PI.png) | ![](scripts/snaps/sdl/where_X_PI.png) | 1.0% |
| 404 | `where_a_b_c` | `A+B$WA$r1` | ![](scripts/snaps/hp/where_a_b_c.png) | ![](scripts/snaps/sdl/where_a_b_c.png) | 0.9% |
| 405 | `where_long` | `X+Y+Z$WX$r1` | ![](scripts/snaps/hp/where_long.png) | ![](scripts/snaps/sdl/where_long.png) | 1.1% |
| 406 | `where_div` | `X/Y$WX$r2` | ![](scripts/snaps/hp/where_div.png) | ![](scripts/snaps/sdl/where_div.png) | 1.0% |
| 407 | `where_pow` | `X^2$WX$r3` | ![](scripts/snaps/hp/where_pow.png) | ![](scripts/snaps/sdl/where_pow.png) | 0.8% |
| 408 | `where_sin` | `X$s$WX$r0` | ![](scripts/snaps/hp/where_sin.png) | ![](scripts/snaps/sdl/where_sin.png) | 1.4% |
| 409 | `where_cos` | `X$c$WX$r0` | ![](scripts/snaps/hp/where_cos.png) | ![](scripts/snaps/sdl/where_cos.png) | 1.4% |
| 410 | `where_const_eq` | `5$WX$r5` | ![](scripts/snaps/hp/where_const_eq.png) | ![](scripts/snaps/sdl/where_const_eq.png) | 0.6% |

### Complex

| # | name | keys | x50ng (HP firmware) | sdleqw | diff |
|---|---|---|---|---|---|
| 411 | `complex_3_4` | `3$Z4` | ![](scripts/snaps/hp/complex_3_4.png) | ![](scripts/snaps/sdl/complex_3_4.png) | 0.5% |
| 412 | `complex_X_Y` | `X$ZY` | ![](scripts/snaps/hp/complex_X_Y.png) | ![](scripts/snaps/sdl/complex_X_Y.png) | 0.6% |
| 413 | `complex_0_1` | `0$Z1` | ![](scripts/snaps/hp/complex_0_1.png) | ![](scripts/snaps/sdl/complex_0_1.png) | 0.4% |
| 414 | `complex_neg_neg` | `-1$Z-1` | ![](scripts/snaps/hp/complex_neg_neg.png) | ![](scripts/snaps/sdl/complex_neg_neg.png) | 0.7% |
| 415 | `complex_decimal` | `1.5$Z2.5` | ![](scripts/snaps/hp/complex_decimal.png) | ![](scripts/snaps/sdl/complex_decimal.png) | 1.1% |
| 416 | `complex_PI_Y` | `PI$ZY` | ![](scripts/snaps/hp/complex_PI_Y.png) | ![](scripts/snaps/sdl/complex_PI_Y.png) | 0.6% |
| 417 | `complex_a_b` | `A$ZB` | ![](scripts/snaps/hp/complex_a_b.png) | ![](scripts/snaps/sdl/complex_a_b.png) | 0.7% |
| 418 | `complex_X_squared_Y` | `X^2$ZY` | ![](scripts/snaps/hp/complex_X_squared_Y.png) | ![](scripts/snaps/sdl/complex_X_squared_Y.png) | 0.8% |
| 419 | `complex_neg_X_Y` | `-X$ZY` | ![](scripts/snaps/hp/complex_neg_X_Y.png) | ![](scripts/snaps/sdl/complex_neg_X_Y.png) | 0.5% |
| 420 | `complex_zero_zero` | `0$Z0` | ![](scripts/snaps/hp/complex_zero_zero.png) | ![](scripts/snaps/sdl/complex_zero_zero.png) | 0.6% |

### User functions

| # | name | keys | x50ng (HP firmware) | sdleqw | diff |
|---|---|---|---|---|---|
| 421 | `uf_F_x` | `F$FX` | ![](scripts/snaps/hp/uf_F_x.png) | ![](scripts/snaps/sdl/uf_F_x.png) | 0.7% |
| 422 | `uf_G_X_Y` | `G$FX,Y` | ![](scripts/snaps/hp/uf_G_X_Y.png) | ![](scripts/snaps/sdl/uf_G_X_Y.png) | 0.8% |
| 423 | `uf_H_a_b_c` | `H$FA,B,R` | ![](scripts/snaps/hp/uf_H_a_b_c.png) | ![](scripts/snaps/sdl/uf_H_a_b_c.png) | 1.1% |
| 424 | `uf_F_2` | `F$F2` | ![](scripts/snaps/hp/uf_F_2.png) | ![](scripts/snaps/sdl/uf_F_2.png) | 0.6% |
| 425 | `uf_g_X_Y_Z` | `g$FX,Y,Z` | ![](scripts/snaps/hp/uf_g_X_Y_Z.png) | ![](scripts/snaps/sdl/uf_g_X_Y_Z.png) | 1.0% |
| 426 | `uf_F_neg_X` | `F$F-X` | ![](scripts/snaps/hp/uf_F_neg_X.png) | ![](scripts/snaps/sdl/uf_F_neg_X.png) | 0.8% |
| 427 | `uf_F_X_plus_Y` | `F$FX+Y` | ![](scripts/snaps/hp/uf_F_X_plus_Y.png) | ![](scripts/snaps/sdl/uf_F_X_plus_Y.png) | 1.0% |
| 428 | `uf_F_X_X` | `F$FX*X` | ![](scripts/snaps/hp/uf_F_X_X.png) | ![](scripts/snaps/sdl/uf_F_X_X.png) | 1.0% |
| 429 | `uf_F_decimal` | `F$F3.14` | ![](scripts/snaps/hp/uf_F_decimal.png) | ![](scripts/snaps/sdl/uf_F_decimal.png) | 1.0% |
| 430 | `uf_F_long_arg` | `F$FVARIABLE` | ![](scripts/snaps/hp/uf_F_long_arg.png) | ![](scripts/snaps/sdl/uf_F_long_arg.png) | 2.0% |

### Edits / deletions

| # | name | keys | x50ng (HP firmware) | sdleqw | diff |
|---|---|---|---|---|---|
| 431 | `edit_bksp_one` | `5$b` | ![](scripts/snaps/hp/edit_bksp_one.png) | ![](scripts/snaps/sdl/edit_bksp_one.png) | 0.0% |
| 432 | `edit_bksp_two` | `12$b` | ![](scripts/snaps/hp/edit_bksp_two.png) | ![](scripts/snaps/sdl/edit_bksp_two.png) | 0.0% |
| 433 | `edit_bksp_three` | `123$b$b` | ![](scripts/snaps/hp/edit_bksp_three.png) | ![](scripts/snaps/sdl/edit_bksp_three.png) | 0.0% |
| 434 | `edit_replace_after_bksp` | `5$bX` | ![](scripts/snaps/hp/edit_replace_after_bksp.png) | ![](scripts/snaps/sdl/edit_replace_after_bksp.png) | 0.0% |
| 435 | `edit_X_then_letter` | `Xy` | ![](scripts/snaps/hp/edit_X_then_letter.png) | ![](scripts/snaps/sdl/edit_X_then_letter.png) | 0.0% |
| 436 | `edit_long_name_then_bksp` | `ABCD$b` | ![](scripts/snaps/hp/edit_long_name_then_bksp.png) | ![](scripts/snaps/sdl/edit_long_name_then_bksp.png) | 0.0% |
| 437 | `edit_5_op_3` | `5+3` | ![](scripts/snaps/hp/edit_5_op_3.png) | ![](scripts/snaps/sdl/edit_5_op_3.png) | 0.0% |
| 438 | `edit_arrow_left` | `5+3$l` | ![](scripts/snaps/hp/edit_arrow_left.png) | ![](scripts/snaps/sdl/edit_arrow_left.png) | 0.9% |
| 439 | `edit_arrow_right` | `5+3$r` | ![](scripts/snaps/hp/edit_arrow_right.png) | ![](scripts/snaps/sdl/edit_arrow_right.png) | 0.8% |
| 440 | `edit_arrow_up` | `5+3$u` | ![](scripts/snaps/hp/edit_arrow_up.png) | ![](scripts/snaps/sdl/edit_arrow_up.png) | 1.0% |
| 441 | `edit_arrow_down` | `5+3$k` | ![](scripts/snaps/hp/edit_arrow_down.png) | ![](scripts/snaps/sdl/edit_arrow_down.png) | 0.9% |
| 442 | `edit_two_arrows` | `5+3$l$l` | ![](scripts/snaps/hp/edit_two_arrows.png) | ![](scripts/snaps/sdl/edit_two_arrows.png) | 0.9% |
| 443 | `edit_three_arrows` | `5+3$l$l$l` | ![](scripts/snaps/hp/edit_three_arrows.png) | ![](scripts/snaps/sdl/edit_three_arrows.png) | 0.9% |
| 444 | `edit_inside_div` | `5/3$u` | ![](scripts/snaps/hp/edit_inside_div.png) | ![](scripts/snaps/sdl/edit_inside_div.png) | 0.9% |
| 445 | `edit_inside_pow` | `X^2$u` | ![](scripts/snaps/hp/edit_inside_pow.png) | ![](scripts/snaps/sdl/edit_inside_pow.png) | 0.9% |
| 446 | `edit_double_bksp` | `12$b$b` | ![](scripts/snaps/hp/edit_double_bksp.png) | ![](scripts/snaps/sdl/edit_double_bksp.png) | 0.0% |
| 447 | `edit_clr_after` | `5+3$K` | ![](scripts/snaps/hp/edit_clr_after.png) | ![](scripts/snaps/sdl/edit_clr_after.png) | 0.6% |
| 448 | `edit_clr_all` | `5+3$L` | ![](scripts/snaps/hp/edit_clr_all.png) | ![](scripts/snaps/sdl/edit_clr_all.png) | 0.6% |
| 449 | `edit_two_then_replace` | `12$bX` | ![](scripts/snaps/hp/edit_two_then_replace.png) | ![](scripts/snaps/sdl/edit_two_then_replace.png) | 0.3% |
| 450 | `edit_select_replace` | `X+5$l$lY` | ![](scripts/snaps/hp/edit_select_replace.png) | ![](scripts/snaps/sdl/edit_select_replace.png) | 0.5% |
| 451 | `edit_5_3_two_arrows` | `5*3$u` | ![](scripts/snaps/hp/edit_5_3_two_arrows.png) | ![](scripts/snaps/sdl/edit_5_3_two_arrows.png) | 0.9% |
| 452 | `edit_complex_seq` | `5+X*2-3` | ![](scripts/snaps/hp/edit_complex_seq.png) | ![](scripts/snaps/sdl/edit_complex_seq.png) | 0.8% |
| 453 | `edit_replace_inside` | `5+3$lY` | ![](scripts/snaps/hp/edit_replace_inside.png) | ![](scripts/snaps/sdl/edit_replace_inside.png) | 0.4% |
| 454 | `edit_arrow_through_div` | `1/2$u` | ![](scripts/snaps/hp/edit_arrow_through_div.png) | ![](scripts/snaps/sdl/edit_arrow_through_div.png) | 1.0% |
| 455 | `edit_arrow_inside_sqrt` | `@X$l` | ![](scripts/snaps/hp/edit_arrow_inside_sqrt.png) | ![](scripts/snaps/sdl/edit_arrow_inside_sqrt.png) | 0.7% |
| 456 | `edit_arrow_inside_paren` | `X(+1$l` | ![](scripts/snaps/hp/edit_arrow_inside_paren.png) | ![](scripts/snaps/sdl/edit_arrow_inside_paren.png) | 1.1% |
| 457 | `edit_after_func` | `X$s$l` | ![](scripts/snaps/hp/edit_after_func.png) | ![](scripts/snaps/sdl/edit_after_func.png) | 1.4% |
| 458 | `edit_typing_after_select` | `5+3$lY` | ![](scripts/snaps/hp/edit_typing_after_select.png) | ![](scripts/snaps/sdl/edit_typing_after_select.png) | 0.4% |
| 459 | `edit_letter_then_bksp` | `abc$b` | ![](scripts/snaps/hp/edit_letter_then_bksp.png) | ![](scripts/snaps/sdl/edit_letter_then_bksp.png) | 0.0% |
| 460 | `edit_simple_overwrite` | `X$bY` | ![](scripts/snaps/hp/edit_simple_overwrite.png) | ![](scripts/snaps/sdl/edit_simple_overwrite.png) | 0.0% |

### Navigation

| # | name | keys | x50ng (HP firmware) | sdleqw | diff |
|---|---|---|---|---|---|
| 461 | `nav_simple_right` | `5+3$r$r` | ![](scripts/snaps/hp/nav_simple_right.png) | ![](scripts/snaps/sdl/nav_simple_right.png) | 0.9% |
| 462 | `nav_simple_left` | `5+3$l$l` | ![](scripts/snaps/hp/nav_simple_left.png) | ![](scripts/snaps/sdl/nav_simple_left.png) | 0.9% |
| 463 | `nav_up_in_div` | `1/2$u$u` | ![](scripts/snaps/hp/nav_up_in_div.png) | ![](scripts/snaps/sdl/nav_up_in_div.png) | 1.7% |
| 464 | `nav_down_in_div` | `1/2$k$k` | ![](scripts/snaps/hp/nav_down_in_div.png) | ![](scripts/snaps/sdl/nav_down_in_div.png) | 0.6% |
| 465 | `nav_around_pow` | `X^2$u$k` | ![](scripts/snaps/hp/nav_around_pow.png) | ![](scripts/snaps/sdl/nav_around_pow.png) | 0.7% |
| 466 | `nav_around_sqrt` | `@X$u` | ![](scripts/snaps/hp/nav_around_sqrt.png) | ![](scripts/snaps/sdl/nav_around_sqrt.png) | 1.2% |
| 467 | `nav_in_func` | `X$s$k` | ![](scripts/snaps/hp/nav_in_func.png) | ![](scripts/snaps/sdl/nav_in_func.png) | 1.4% |
| 468 | `nav_in_func_args` | `F$FX,Y$l` | ![](scripts/snaps/hp/nav_in_func_args.png) | ![](scripts/snaps/sdl/nav_in_func_args.png) | 1.1% |
| 469 | `nav_around_paren` | `X+Y($u` | ![](scripts/snaps/hp/nav_around_paren.png) | ![](scripts/snaps/sdl/nav_around_paren.png) | 1.7% |
| 470 | `nav_through_complex` | `3$Z4$l$r` | ![](scripts/snaps/hp/nav_through_complex.png) | ![](scripts/snaps/sdl/nav_through_complex.png) | 1.0% |
| 471 | `nav_in_integ` | `$I0$rX$rX$rX$l` | ![](scripts/snaps/hp/nav_in_integ.png) | ![](scripts/snaps/sdl/nav_in_integ.png) | 1.6% |
| 472 | `nav_in_sum` | `$Sk$r1$rn$rk$l` | ![](scripts/snaps/hp/nav_in_sum.png) | ![](scripts/snaps/sdl/nav_in_sum.png) | 1.2% |
| 473 | `nav_through_long_expr` | `5+3*4$l` | ![](scripts/snaps/hp/nav_through_long_expr.png) | ![](scripts/snaps/sdl/nav_through_long_expr.png) | 1.1% |
| 474 | `nav_after_left` | `5+3$l` | ![](scripts/snaps/hp/nav_after_left.png) | ![](scripts/snaps/sdl/nav_after_left.png) | 0.9% |
| 475 | `nav_after_right` | `5+3$r` | ![](scripts/snaps/hp/nav_after_right.png) | ![](scripts/snaps/sdl/nav_after_right.png) | 0.8% |
| 476 | `nav_after_two_lefts` | `5+3$l$l` | ![](scripts/snaps/hp/nav_after_two_lefts.png) | ![](scripts/snaps/sdl/nav_after_two_lefts.png) | 0.8% |
| 477 | `nav_after_three_rights` | `5+3$r$r$r` | ![](scripts/snaps/hp/nav_after_three_rights.png) | ![](scripts/snaps/sdl/nav_after_three_rights.png) | 0.9% |
| 478 | `nav_in_neg` | `-5$u` | ![](scripts/snaps/hp/nav_in_neg.png) | ![](scripts/snaps/sdl/nav_in_neg.png) | 0.8% |
| 479 | `nav_back_to_root` | `X+Y+Z$u$u$u` | ![](scripts/snaps/hp/nav_back_to_root.png) | ![](scripts/snaps/sdl/nav_back_to_root.png) | 1.7% |
| 480 | `nav_first_arg` | `5+3$l$l$l` | ![](scripts/snaps/hp/nav_first_arg.png) | ![](scripts/snaps/sdl/nav_first_arg.png) | 0.8% |
| 481 | `nav_last_arg` | `5+3*4$r$r$r` | ![](scripts/snaps/hp/nav_last_arg.png) | ![](scripts/snaps/sdl/nav_last_arg.png) | 1.2% |
| 482 | `nav_nested_div_up` | `1/X+Y$u` | ![](scripts/snaps/hp/nav_nested_div_up.png) | ![](scripts/snaps/sdl/nav_nested_div_up.png) | 1.1% |
| 483 | `nav_after_paren_wrap` | `X+Y($u` | ![](scripts/snaps/hp/nav_after_paren_wrap.png) | ![](scripts/snaps/sdl/nav_after_paren_wrap.png) | 1.7% |
| 484 | `nav_in_abs` | `X|$u` | ![](scripts/snaps/hp/nav_in_abs.png) | ![](scripts/snaps/sdl/nav_in_abs.png) | 1.3% |
| 485 | `nav_in_complex` | `5$Z4$u` | ![](scripts/snaps/hp/nav_in_complex.png) | ![](scripts/snaps/sdl/nav_in_complex.png) | 1.5% |
| 486 | `nav_in_two_arg_func` | `F$FX,Y$l` | ![](scripts/snaps/hp/nav_in_two_arg_func.png) | ![](scripts/snaps/sdl/nav_in_two_arg_func.png) | 1.1% |
| 487 | `nav_in_three_arg_func` | `F$FX,Y,Z$l` | ![](scripts/snaps/hp/nav_in_three_arg_func.png) | ![](scripts/snaps/sdl/nav_in_three_arg_func.png) | 1.3% |
| 488 | `nav_after_userfunc` | `F$FX,Y$r` | ![](scripts/snaps/hp/nav_after_userfunc.png) | ![](scripts/snaps/sdl/nav_after_userfunc.png) | 1.1% |
| 489 | `nav_in_pow_after_arrows` | `X^2$u$k` | ![](scripts/snaps/hp/nav_in_pow_after_arrows.png) | ![](scripts/snaps/sdl/nav_in_pow_after_arrows.png) | 0.7% |
| 490 | `nav_complex_path` | `1+2*3$u$u$k` | ![](scripts/snaps/hp/nav_complex_path.png) | ![](scripts/snaps/sdl/nav_complex_path.png) | 1.0% |

### Auto-multiply

| # | name | keys | x50ng (HP firmware) | sdleqw | diff |
|---|---|---|---|---|---|
| 491 | `auto_2X` | `2X` | ![](scripts/snaps/hp/auto_2X.png) | ![](scripts/snaps/sdl/auto_2X.png) | 0.0% |
| 492 | `auto_3PI` | `3PI` | ![](scripts/snaps/hp/auto_3PI.png) | ![](scripts/snaps/sdl/auto_3PI.png) | 0.0% |
| 493 | `auto_5L` | `5L` | ![](scripts/snaps/hp/auto_5L.png) | ![](scripts/snaps/sdl/auto_5L.png) | 0.0% |
| 494 | `auto_2X_again` | `2X` | ![](scripts/snaps/hp/auto_2X_again.png) | ![](scripts/snaps/sdl/auto_2X_again.png) | 0.0% |
| 495 | `auto_decimal` | `0.5X` | ![](scripts/snaps/hp/auto_decimal.png) | ![](scripts/snaps/sdl/auto_decimal.png) | 0.0% |
| 496 | `auto_3R` | `3R` | ![](scripts/snaps/hp/auto_3R.png) | ![](scripts/snaps/sdl/auto_3R.png) | 0.0% |
| 497 | `auto_2X_3` | `2X*3` | ![](scripts/snaps/hp/auto_2X_3.png) | ![](scripts/snaps/sdl/auto_2X_3.png) | 0.0% |
| 498 | `auto_2L` | `2L` | ![](scripts/snaps/hp/auto_2L.png) | ![](scripts/snaps/sdl/auto_2L.png) | 0.0% |
| 499 | `auto_4_OMEGA` | `4OMEGA` | ![](scripts/snaps/hp/auto_4_OMEGA.png) | ![](scripts/snaps/sdl/auto_4_OMEGA.png) | 0.0% |
| 500 | `auto_X_then_2` | `X2` | ![](scripts/snaps/hp/auto_X_then_2.png) | ![](scripts/snaps/sdl/auto_X_then_2.png) | 0.0% |
| 501 | `auto_Xy` | `Xy` | ![](scripts/snaps/hp/auto_Xy.png) | ![](scripts/snaps/sdl/auto_Xy.png) | 0.0% |
| 502 | `auto_2X_plus_Y` | `2X+Y` | ![](scripts/snaps/hp/auto_2X_plus_Y.png) | ![](scripts/snaps/sdl/auto_2X_plus_Y.png) | 0.4% |
| 503 | `auto_3X_squared` | `3X^2` | ![](scripts/snaps/hp/auto_3X_squared.png) | ![](scripts/snaps/sdl/auto_3X_squared.png) | 0.0% |
| 504 | `auto_2_minus_X` | `2-X` | ![](scripts/snaps/hp/auto_2_minus_X.png) | ![](scripts/snaps/sdl/auto_2_minus_X.png) | 0.0% |
| 505 | `auto_3PI_R` | `3PI*R` | ![](scripts/snaps/hp/auto_3PI_R.png) | ![](scripts/snaps/sdl/auto_3PI_R.png) | 0.5% |

### Doc examples

| # | name | keys | x50ng (HP firmware) | sdleqw | diff |
|---|---|---|---|---|---|
| 506 | `doc_meta_kernel_integ` | `E$I0$r1/X$rABS$r$rt` | ![](scripts/snaps/hp/doc_meta_kernel_integ.png) | ![](scripts/snaps/sdl/doc_meta_kernel_integ.png) | 1.2% |
| 507 | `doc_user_guide_arith` | `5*1+1/7.5$r/@3-2^3` | ![](scripts/snaps/hp/doc_user_guide_arith.png) | ![](scripts/snaps/sdl/doc_user_guide_arith.png) | 3.1% |
| 508 | `doc_user_guide_algebra` | `2*L*@1+X/R$r/R+y$r+2*L/b` | ![](scripts/snaps/hp/doc_user_guide_algebra.png) | ![](scripts/snaps/sdl/doc_user_guide_algebra.png) | 4.0% |
| 509 | `doc_user_guide_greek` | `2/@3$rL+e^-Mu*LNX+2*Mu*Y$r/THETA^1/3` | ![](scripts/snaps/hp/doc_user_guide_greek.png) | ![](scripts/snaps/sdl/doc_user_guide_greek.png) | 6.1% |
| 510 | `doc_pythagorean` | `@A^2+B^2` | ![](scripts/snaps/hp/doc_pythagorean.png) | ![](scripts/snaps/sdl/doc_pythagorean.png) | 2.0% |
| 511 | `doc_sin_cos_sq` | `X$s^2+X$c^2` | ![](scripts/snaps/hp/doc_sin_cos_sq.png) | ![](scripts/snaps/sdl/doc_sin_cos_sq.png) | 3.1% |
| 512 | `doc_quadratic` | `A*X^2+B*X+R` | ![](scripts/snaps/hp/doc_quadratic.png) | ![](scripts/snaps/sdl/doc_quadratic.png) | 1.6% |
| 513 | `doc_simple_geom` | `PI*R^2` | ![](scripts/snaps/hp/doc_simple_geom.png) | ![](scripts/snaps/sdl/doc_simple_geom.png) | 0.3% |
| 514 | `doc_volume` | `4/3*PI*R^3` | ![](scripts/snaps/hp/doc_volume.png) | ![](scripts/snaps/sdl/doc_volume.png) | 0.4% |
| 515 | `doc_simple_log` | `X$n+Y$n` | ![](scripts/snaps/hp/doc_simple_log.png) | ![](scripts/snaps/sdl/doc_simple_log.png) | 1.8% |

## x50ng patch

The x50ng modification adds a self-contained scripting hook in `src/main.c`. It activates when `X50NG_SCRIPT=<file>` is in the environment. The patch is small (~150 LoC) and only touches `src/main.c`.

Script commands:
  - `WAIT <ticks>` — virtual idle for `ticks × 30 ms`.
  - `PRESS <KEY>` / `RELEASE <KEY>` — single edge.
  - `TAP <KEY>` — press, hold ~120 ms, release, settle ~120 ms.
  - `SNAP <name>` — write `<X50NG_SNAP_PREFIX>_NNN_<name>.pgm`.
  - `QUIT` — call `hdw_stop()`.

Key names are HP4950 enum suffixes: `A..Z`, `0..9`, `PLUS`, `MINUS`, `MULTIPLY`, `ENTER`, `BACKSPACE`, `ALPHA`, `LEFTSHIFT`, `RIGHTSHIFT`, `ON`, `PERIOD`, `SPACE`, `UP`, `DOWN`, `LEFT`, `RIGHT`.

