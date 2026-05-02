#!/usr/bin/env python3
"""
render_abx.py — produce side-by-side comparison images in abx/images/.

For each test, finds the HP and sdleqw PGM, scales each to 524×320, and
composites them horizontally with a header bar (test name, keys, diff %).
Output: abx/images/<NNNN>_<name>.png
"""
import os, sys, csv, subprocess

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
OUT = os.path.join(ROOT, "abx", "images")
os.makedirs(OUT, exist_ok=True)

# Pre-import diff_score
sys.path.insert(0, os.path.join(ROOT, "tools"))
from build_ref import diff_score, find  # noqa: E402

SUITES = [
    ("main",    "scripts/tests.csv",         "/tmp/eqw_test/hp",      "/tmp/eqw_test/sdl"),
    ("complex", "scripts/complex_tests.csv", "/tmp/eqw_test/cmpl_hp", "/tmp/eqw_test/cmpl_sdl"),
]

def compose(hp_pgm, sdl_pgm, out_path, header):
    """Compose a side-by-side comparison image with header."""
    # Stage 1: scale each PGM to 524x320 PNG
    hp_png = "/tmp/_abx_hp.png"
    sdl_png = "/tmp/_abx_sdl.png"
    subprocess.run(["convert", hp_pgm, "-scale", "524x320", hp_png], check=True,
                   stderr=subprocess.DEVNULL)
    subprocess.run(["convert", sdl_pgm, "-scale", "524x320", sdl_png], check=True,
                   stderr=subprocess.DEVNULL)
    # Stage 2: add a 1-line header bar to each, then horizontally append
    hp_label = "/tmp/_abx_hp_lbl.png"
    sdl_label = "/tmp/_abx_sdl_lbl.png"
    subprocess.run(["convert", hp_png,
                    "-bordercolor", "#444", "-border", "1x1",
                    "-gravity", "north",
                    "-background", "#222", "-fill", "white",
                    "-pointsize", "16", "-font", "DejaVu-Sans-Mono",
                    "label:HP\\ 50g\\ (x50ng)",
                    "+swap", "-append",
                    hp_label], check=True, stderr=subprocess.DEVNULL)
    subprocess.run(["convert", sdl_png,
                    "-bordercolor", "#444", "-border", "1x1",
                    "-gravity", "north",
                    "-background", "#222", "-fill", "white",
                    "-pointsize", "16", "-font", "DejaVu-Sans-Mono",
                    "label:sdleqw",
                    "+swap", "-append",
                    sdl_label], check=True, stderr=subprocess.DEVNULL)
    # Stage 3: horizontally append, with gap
    side_by_side = "/tmp/_abx_sbs.png"
    subprocess.run(["convert", hp_label, sdl_label, "+append",
                    "-bordercolor", "#222", "-border", "8x4",
                    side_by_side], check=True, stderr=subprocess.DEVNULL)
    # Stage 4: prepend header bar with the test info
    subprocess.run(["convert",
                    "-size", "1090x40",
                    "-background", "#222",
                    "-fill", "#ddd",
                    "-font", "DejaVu-Sans-Mono",
                    "-pointsize", "14",
                    "-gravity", "west",
                    f"caption:  {header}",
                    side_by_side, "-append",
                    out_path], check=True, stderr=subprocess.DEVNULL)

count = 0
for suite_name, csv_path, hp_dir, sdl_dir in SUITES:
    csv_full = os.path.join(ROOT, csv_path)
    if not os.path.exists(csv_full):
        print(f"skip {suite_name} — no csv", file=sys.stderr)
        continue
    for row in csv.DictReader(open(csv_full)):
        name = row["name"]
        keys = row["keys"]
        idx  = int(row["idx"])
        hp = find(hp_dir, name)
        sd = find(sdl_dir, name)
        if not hp or not sd:
            print(f"skip {suite_name}/{name} — missing snapshot", file=sys.stderr)
            continue
        d = diff_score(hp, sd)
        d_str = f"{d*100:.2f}%" if d is not None else "?"
        prefix = "M" if suite_name == "main" else "C"
        out_name = f"{prefix}{idx:04d}_{name}.png"
        out_path = os.path.join(OUT, out_name)
        header = f"[{suite_name} #{idx}] {name}  keys: {keys}    diff: {d_str}"
        try:
            compose(hp, sd, out_path, header)
            count += 1
            if count % 50 == 0:
                print(f"  rendered {count} comparisons...", file=sys.stderr)
        except subprocess.CalledProcessError as e:
            print(f"!! convert failed for {name}: {e}", file=sys.stderr)

print(f"done — {count} comparison images in {OUT}/")
