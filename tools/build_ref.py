#!/usr/bin/env python3
"""
build_ref.py — assemble REF.md from the per-test PGMs of both x50ng and sdleqw.

Inputs:
  /tmp/eqw_test/hp/h_NNNN_<name>.pgm     (from x50ng)
  /tmp/eqw_test/sdl/s_NNNN_<name>.pgm    (from sdleqw --batch)
  scripts/tests.csv

Outputs:
  REF.md
  scripts/snaps/{hp,sdl}/<name>.png      (PNGs for embedded display)

The match is done by the test `name`, since the two scripts produce snapshots
in the same order — index NNNN should match.
"""

import csv, os, subprocess, sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
HP_DIR  = "/tmp/eqw_test/hp"
SDL_DIR = "/tmp/eqw_test/sdl"
SNAPS = os.path.join(ROOT, "scripts", "snaps")
HP_OUT  = os.path.join(SNAPS, "hp")
SDL_OUT = os.path.join(SNAPS, "sdl")
os.makedirs(HP_OUT,  exist_ok=True)
os.makedirs(SDL_OUT, exist_ok=True)

def find(d, name):
    """Find <prefix>_NNNN_<name>.pgm in directory d, return full path or None."""
    for f in os.listdir(d):
        if f.endswith(f"_{name}.pgm"):
            return os.path.join(d, f)
    return None

def pgm_to_png(src, dst, scale_w=262, scale_h=160):
    if not src or not os.path.exists(src):
        return False
    if os.path.exists(dst):
        return True
    # use pnmtopng then optional scale via convert; or use convert directly
    try:
        subprocess.check_call(["convert", src, "-scale", f"{scale_w}x{scale_h}", dst],
                              stderr=subprocess.DEVNULL)
        return True
    except subprocess.CalledProcessError:
        return False

def pgm_pixels(path):
    """Read raw P5 PGM. Returns (w, h, bytes) or None."""
    if not path or not os.path.exists(path):
        return None
    with open(path, "rb") as f:
        line = f.readline()
        if not line.startswith(b"P5"): return None
        # skip comments
        line = f.readline()
        while line.startswith(b"#"): line = f.readline()
        w, h = map(int, line.split())
        line = f.readline()  # maxval
        data = f.read(w * h)
    return (w, h, data)

def diff_score(p1, p2, crop_top=0, crop_bot=0):
    """Fraction of pixels that differ (0..1) in [crop_top..h-crop_bot)."""
    a = pgm_pixels(p1)
    b = pgm_pixels(p2)
    if a is None or b is None:
        return None
    if a[:2] != b[:2]:
        return None
    w, h = a[0], a[1]
    y0, y1 = crop_top, h - crop_bot
    if y1 <= y0: return None
    diff = 0
    for y in range(y0, y1):
        for x in range(w):
            af = 1 if a[2][y * w + x] < 0x80 else 0
            bf = 1 if b[2][y * w + x] < 0x80 else 0
            if af != bf: diff += 1
    return diff / (w * (y1 - y0))

# Equation-area-only diff (crop the bottom 7-row menu strip).
def diff_score_eq(p1, p2):
    return diff_score(p1, p2, crop_top=0, crop_bot=7)

# --- read tests.csv ---
tests = []
with open(os.path.join(ROOT, "scripts", "tests.csv")) as f:
    for row in csv.DictReader(f):
        tests.append(row)

# --- generate snapshot PNGs ---
print("converting snapshots...", file=sys.stderr)
for t in tests:
    name = t["name"]
    hp = find(HP_DIR, name)
    sdl = find(SDL_DIR, name)
    if hp:
        pgm_to_png(hp, os.path.join(HP_OUT, f"{name}.png"))
    if sdl:
        pgm_to_png(sdl, os.path.join(SDL_OUT, f"{name}.png"))

# --- generate REF.md ---
print("writing REF.md ...", file=sys.stderr)
out = []
out.append("# sdleqw vs x50ng (HP 50g firmware) — functional validation\n")
out.append("\n")
out.append("This file is the reference comparison between **sdleqw** (this repository's pure-C reimplementation of the HP 50g Equation Writer) and **x50ng** (the official HP49g+/50g hardware emulator running the genuine HP firmware ROM).\n\n")

out.append("## Setup\n\n")
out.append("- **x50ng** built locally from `~/github/x50ng/` with a small patch in `src/main.c` that adds a headless scripting hook (`X50NG_SCRIPT`, `X50NG_SNAP_PREFIX` env vars). The patch ([diff](#x50ng-patch)) drives `press_key()` / `release_key()` from a script and snapshots the LCD via `get_lcd_buffer()`.\n")
out.append("- **sdleqw** built normally (`make`) and run with `--batch SCRIPT --snap-prefix PFX`.\n")
out.append("- Both binaries run **headlessly** (no SDL window required for x50ng either — it uses the ncurses TUI frontend purely as a host loop, but the LCD bytes come straight from `get_lcd_buffer`).\n")
out.append("- The same logical key sequence is fed to each. The mini-language used to author the tests is documented in `tools/gen_tests.py`. For each test, `gen_tests.py` emits an x50ng-flavoured key sequence (with shifts and alpha-locking) and an sdleqw-flavoured key sequence (one EQW action per char).\n")
out.append("- Each captured frame is a 131×80 bitmap. The HP LCD buffer comes back as 0..15 grayscale; we threshold to 1-bpp.\n\n")

out.append("## Test cycle\n\n")
out.append("```\n")
out.append("# x50ng test cycle:\n")
out.append("WAIT 100             # cold boot to home screen\n")
out.append("SNAP boot            # baseline\n")
out.append("# for each test:\n")
out.append("TAP RIGHTSHIFT       # enter EQW: RS+QUOTE\n")
out.append("TAP O\n")
out.append("WAIT 25\n")
out.append("<keys for the test expression>\n")
out.append("WAIT 20\n")
out.append("SNAP <name>\n")
out.append("TAP ON               # cancel EQW back to home\n")
out.append("WAIT 15\n")
out.append("```\n\n")

out.append("## Results summary\n\n")

# Compute summary stats
total = len(tests)
both = 0
hp_only = 0
sdl_only = 0
neither = 0
diff_buckets = {"<0.5%": 0, "0.5-1%": 0, "1-2%": 0, "2-3%": 0, "3-5%": 0, "5-10%": 0, ">10%": 0}
for t in tests:
    name = t["name"]
    hp_png = os.path.join(HP_OUT, f"{name}.png")
    sdl_png = os.path.join(SDL_OUT, f"{name}.png")
    h_ok = os.path.exists(hp_png)
    s_ok = os.path.exists(sdl_png)
    if h_ok and s_ok:
        both += 1
        hp_pgm = find(HP_DIR, name)
        sdl_pgm = find(SDL_DIR, name)
        d = diff_score(hp_pgm, sdl_pgm)
        if d is not None:
            if   d < 0.005: diff_buckets["<0.5%"] += 1
            elif d < 0.01:  diff_buckets["0.5-1%"] += 1
            elif d < 0.02:  diff_buckets["1-2%"] += 1
            elif d < 0.03:  diff_buckets["2-3%"] += 1
            elif d < 0.05:  diff_buckets["3-5%"] += 1
            elif d < 0.10:  diff_buckets["5-10%"] += 1
            else:           diff_buckets[">10%"] += 1
    elif h_ok: hp_only += 1
    elif s_ok: sdl_only += 1
    else: neither += 1

out.append(f"- **Total tests:** {total}\n")
out.append(f"- **Both produced a snap:** {both}\n")
out.append(f"- x50ng-only: {hp_only},  sdleqw-only: {sdl_only},  neither: {neither}\n\n")
# Also compute equation-only (cropped) diff
eq_buckets = {"<0.5%": 0, "0.5-1%": 0, "1-2%": 0, "2-3%": 0, "3-5%": 0, "5-10%": 0, ">10%": 0}
eq_total = 0
for t in tests:
    name = t["name"]
    hp_pgm = find(HP_DIR, name)
    sdl_pgm = find(SDL_DIR, name)
    if hp_pgm and sdl_pgm:
        d = diff_score_eq(hp_pgm, sdl_pgm)
        if d is not None:
            eq_total += 1
            if   d < 0.005: eq_buckets["<0.5%"] += 1
            elif d < 0.01:  eq_buckets["0.5-1%"] += 1
            elif d < 0.02:  eq_buckets["1-2%"] += 1
            elif d < 0.03:  eq_buckets["2-3%"] += 1
            elif d < 0.05:  eq_buckets["3-5%"] += 1
            elif d < 0.10:  eq_buckets["5-10%"] += 1
            else:           eq_buckets[">10%"] += 1

out.append("**Pixel-difference distribution** (binary thresholded; lower = more similar):\n\n")
out.append("Full LCD (131×80) including menu strip:\n\n")
out.append("| bucket | count | share |\n|---|---|---|\n")
for k, v in diff_buckets.items():
    pct = (100.0 * v / both) if both else 0
    out.append(f"| {k} | {v} | {pct:.1f}% |\n")
out.append("\n")
out.append("Equation area only (rows 0..72, menu strip cropped out):\n\n")
out.append("| bucket | count | share |\n|---|---|---|\n")
for k, v in eq_buckets.items():
    pct = (100.0 * v / eq_total) if eq_total else 0
    out.append(f"| {k} | {v} | {pct:.1f}% |\n")
out.append("\n")
out.append("> **What the diff measures.** Each PGM is binary-thresholded; we count the\n> pixels that disagree between HP firmware and sdleqw, divided by total\n> pixels in the comparison region. 0% = pixel-perfect match. The diff\n> is a ranking signal, not a pass/fail. \n\n")
out.append("> **How sdleqw was tuned to close the gap.** Iteratively:\n> 1. Imported HP's variable-width 5×7 stack font glyphs from x50ng snapshots\n>    (`tools/gen_hp_font.py`).\n> 2. Imported HP's exact menu-strip pixel layout (131×7 bitmap, `src/hp_menu.c`).\n> 3. Adjusted vertical centering to use the (LCD_H - MENU_H)=73-row equation area.\n> 4. Adjusted horizontal centering to HP's slightly-left-of-center axis (col 62).\n> 5. Removed the spurious 4-pixel minimum-width clamp on glyph layout that\n>    was widening narrow glyphs like '1'.\n> 6. Added 1-pixel gaps above and below division bars to match HP's spacing.\n\n")

out.append("## Per-test comparisons\n\n")

# Group by category prefix.
def category(name):
    pref = name.split("_", 1)[0]
    return {
        "num": "Numbers", "name": "Names",
        "add": "Addition", "sub": "Subtraction",
        "mul": "Multiplication", "div": "Division",
        "pow": "Powers", "sqrt": "Square roots",
        "nthroot": "Nth roots", "paren": "Parens",
        "abs": "Absolute value", "sin": "Prefix functions",
        "cos": "Prefix functions", "tan": "Prefix functions",
        "ln":  "Prefix functions", "exp": "Prefix functions",
        "log": "Prefix functions",
        "integ": "Integrals", "sum": "Summations",
        "deriv": "Derivatives", "where": "Where",
        "complex": "Complex", "uf": "User functions",
        "edit": "Edits / deletions", "nav": "Navigation",
        "auto": "Auto-multiply", "doc": "Doc examples",
    }.get(pref, "Other")

current_cat = None
for t in tests:
    name = t["name"]
    keys = t["keys"]
    cat = category(name)
    if cat != current_cat:
        out.append(f"\n### {cat}\n\n")
        out.append("| # | name | keys | x50ng (HP firmware) | sdleqw | diff |\n")
        out.append("|---|---|---|---|---|---|\n")
        current_cat = cat
    hp_png = f"scripts/snaps/hp/{name}.png"
    sdl_png = f"scripts/snaps/sdl/{name}.png"
    hp_exists = os.path.exists(os.path.join(ROOT, hp_png))
    sdl_exists = os.path.exists(os.path.join(ROOT, sdl_png))
    hp_cell = f"![]({hp_png})" if hp_exists else "_(missing)_"
    sdl_cell = f"![]({sdl_png})" if sdl_exists else "_(missing)_"
    if hp_exists and sdl_exists:
        d = diff_score(find(HP_DIR, name), find(SDL_DIR, name))
        if d is None: dlabel = "n/a"
        else: dlabel = f"{d*100:.1f}%"
    else:
        dlabel = "—"
    out.append(f"| {t['idx']} | `{name}` | `{keys}` | {hp_cell} | {sdl_cell} | {dlabel} |\n")

out.append("\n## x50ng patch\n\n")
out.append("The x50ng modification adds a self-contained scripting hook in `src/main.c`. ")
out.append("It activates when `X50NG_SCRIPT=<file>` is in the environment. The patch is small (~150 LoC) and only touches `src/main.c`.\n\n")
out.append("Script commands:\n")
out.append("  - `WAIT <ticks>` — virtual idle for `ticks × 30 ms`.\n")
out.append("  - `PRESS <KEY>` / `RELEASE <KEY>` — single edge.\n")
out.append("  - `TAP <KEY>` — press, hold ~120 ms, release, settle ~120 ms.\n")
out.append("  - `SNAP <name>` — write `<X50NG_SNAP_PREFIX>_NNN_<name>.pgm`.\n")
out.append("  - `QUIT` — call `hdw_stop()`.\n\n")
out.append("Key names are HP4950 enum suffixes: `A..Z`, `0..9`, `PLUS`, `MINUS`, `MULTIPLY`, `ENTER`, `BACKSPACE`, `ALPHA`, `LEFTSHIFT`, `RIGHTSHIFT`, `ON`, `PERIOD`, `SPACE`, `UP`, `DOWN`, `LEFT`, `RIGHT`.\n\n")

with open(os.path.join(ROOT, "REF.md"), "w") as f:
    f.write("".join(out))
print(f"REF.md written ({both}/{total} matched)")
