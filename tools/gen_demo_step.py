#!/usr/bin/env python3
"""
gen_demo_step.py — emit the x50ng-side step-by-step script for the
demo_arith ABX comparison. Snaps after each individual keypress.
"""
import os, sys
HERE = os.path.dirname(__file__)
sys.path.insert(0, HERE)
from gen_tests import X50Builder

# Logical step list: (label, keychars).  Each step is rendered as a SNAP
# with that label after lowering the keys for x50ng.
STEPS = [
    ("00_init",  ""),
    ("01_5",     "5"),
    ("02_mul",   "*"),
    ("03_1",     "1"),
    ("04_plus",  "+"),
    ("05_1",     "1"),
    ("06_div",   "/"),
    ("07_7",     "7"),
    ("08_period","."),
    ("09_5",     "5"),
    ("10_right", "$r"),
    ("11_div",   "/"),
    ("12_sqrt",  "@"),
    ("13_3",     "3"),
    ("14_minus", "-"),
    ("15_2",     "2"),
    ("16_pow",   "^"),
    ("17_3",     "3"),
]

def emit_x50():
    out = []
    out.append("# demo_arith — per-keystroke snapshots.")
    out.append("WAIT 100")
    out.append("SNAP boot")
    out.append("")
    out.append("# Enter EQW")
    out.append("TAP RIGHTSHIFT")
    out.append("TAP O")
    out.append("WAIT 25")
    for label, keys in STEPS:
        if keys:
            b = X50Builder()
            b.lower(keys)
            for ln in b.lines:
                out.append(ln)
        out.append("WAIT 12")
        out.append(f"SNAP demo_arith_{label}")
    out.append("TAP ON")
    out.append("WAIT 15")
    return "\n".join(out) + "\n"

if __name__ == "__main__":
    path = os.path.join(HERE, "..", "scripts", "demo_step_x50ng.script")
    with open(path, "w") as f:
        f.write(emit_x50())
    print(f"wrote {path}")
