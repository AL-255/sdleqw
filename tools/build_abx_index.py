#!/usr/bin/env python3
"""Build abx/index.md — clickable index of side-by-side comparison images."""
import os, sys, csv

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
ABX = os.path.join(ROOT, "abx")
IMG = os.path.join(ABX, "images")

sys.path.insert(0, os.path.join(ROOT, "tools"))
from build_ref import diff_score, find  # noqa: E402

SUITES = [
    ("Main test suite (516 cases)",   "M", "scripts/tests.csv",         "/tmp/eqw_test/hp",      "/tmp/eqw_test/sdl"),
    ("Complex expressions (25 cases)", "C", "scripts/complex_tests.csv", "/tmp/eqw_test/cmpl_hp", "/tmp/eqw_test/cmpl_sdl"),
]

out = []
out.append("# sdleqw vs HP 50g — side-by-side image gallery\n")
out.append("\n")
out.append("Each image is a horizontal pair: **HP firmware (x50ng)** on the left, **sdleqw** on the right.\n")
out.append("Header shows the suite, test #, name, key sequence, and binary pixel-diff percentage.\n\n")
out.append("![sample](images/C0001_pythagoras.png)\n\n")
out.append("All %d comparison images live in `abx/images/`. Index by suite below.\n\n" %
           sum(1 for _ in os.listdir(IMG) if _.endswith(".png")))

# Categorize
def category(name):
    pref = name.split("_", 1)[0]
    return {
        "num": "Numbers", "name": "Names", "add": "Addition", "sub": "Subtraction",
        "mul": "Multiplication", "div": "Division", "pow": "Powers",
        "sqrt": "Square roots", "nthroot": "Nth roots", "paren": "Parens",
        "abs": "Absolute value",
        "sin": "Prefix functions", "cos": "Prefix functions", "tan": "Prefix functions",
        "ln":  "Prefix functions", "exp": "Prefix functions", "log": "Prefix functions",
        "absfn": "Prefix functions",
        "integ": "Integrals", "sum": "Summations", "deriv": "Derivatives",
        "where": "Where", "complex": "Complex", "uf": "User functions",
        "edit": "Edits / deletions", "nav": "Navigation",
        "auto": "Auto-multiply", "doc": "Doc examples",
    }.get(pref, "Other")

for title, prefix, csv_path, hp_dir, sdl_dir in SUITES:
    csv_full = os.path.join(ROOT, csv_path)
    if not os.path.exists(csv_full):
        continue
    out.append(f"\n## {title}\n\n")
    rows = list(csv.DictReader(open(csv_full)))
    # Group by category
    by_cat = {}
    for row in rows:
        cat = category(row["name"]) if prefix == "M" else "Complex"
        by_cat.setdefault(cat, []).append(row)

    for cat in sorted(by_cat.keys()):
        out.append(f"\n### {cat}\n\n")
        out.append("| # | name | keys | diff | image |\n|---|---|---|---|---|\n")
        for row in by_cat[cat]:
            name = row["name"]
            keys = row["keys"]
            idx = int(row["idx"])
            hp = find(hp_dir, name); sd = find(sdl_dir, name)
            d = diff_score(hp, sd) if (hp and sd) else None
            d_str = f"{d*100:.2f}%" if d is not None else "—"
            img = f"images/{prefix}{idx:04d}_{name}.png"
            if not os.path.exists(os.path.join(ABX, img)):
                continue
            out.append(f"| {idx} | `{name}` | `{keys}` | {d_str} | [![]({img})]({img}) |\n")

with open(os.path.join(ABX, "index.md"), "w") as f:
    f.write("".join(out))
print(f"wrote {os.path.join(ABX, 'index.md')}")
