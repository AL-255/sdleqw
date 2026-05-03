#!/usr/bin/env python3
"""
diff_demo_step.py — compare per-keystroke HP and SDL PGMs and report diffs.
"""
import os, sys

OUT_HP  = "/tmp/demo_step/hp"
OUT_SDL = "/tmp/demo_step/sdl"

def find(d, suffix):
    for f in sorted(os.listdir(d)):
        if f.endswith(f"_{suffix}.pgm"):
            return os.path.join(d, f)
    return None

def read_pgm(path):
    if not path or not os.path.exists(path): return None
    with open(path, "rb") as f:
        line = f.readline()
        if not line.startswith(b"P5"): return None
        line = f.readline()
        while line.startswith(b"#"): line = f.readline()
        w, h = map(int, line.split())
        f.readline()  # maxval
        data = f.read()
        return (w, h, data)

def diff(p1, p2):
    a, b = read_pgm(p1), read_pgm(p2)
    if a is None or b is None: return ("missing", 0, 0)
    if (a[0], a[1]) != (b[0], b[1]): return ("size", a, b)
    d = sum(1 for x, y in zip(a[2], b[2]) if x != y)
    return ("ok", d, len(a[2]))

STEPS = [
    "00_init", "01_5", "02_mul", "03_1", "04_plus", "05_1", "06_div",
    "07_7", "08_period", "09_5", "10_right", "11_div", "12_sqrt",
    "13_3", "14_minus", "15_2", "16_pow", "17_3",
]

print(f"{'step':<14} {'hp':<48} {'sdl':<48} {'diff_px':>8}  {'pct':>6}")
print("-" * 130)
total = 0
total_perfect = 0
for step in STEPS:
    name = f"demo_arith_{step}"
    hp_p = find(OUT_HP, name)
    sd_p = find(OUT_SDL, name)
    status, d, size = diff(hp_p, sd_p)
    if status == "missing":
        print(f"{step:<14} MISSING")
        continue
    if status == "size":
        print(f"{step:<14} SIZE MISMATCH hp={d} sdl={size}")
        continue
    pct = (d * 100.0 / size) if size else 0
    total += d
    if d == 0: total_perfect += 1
    print(f"{step:<14} {os.path.basename(hp_p):<48} {os.path.basename(sd_p):<48} {d:>8}  {pct:>5.2f}%")
print("-" * 130)
print(f"perfect: {total_perfect}/{len(STEPS)}, total diff: {total}")
