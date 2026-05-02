#!/usr/bin/env python3
"""Extract HP's mini font from the soft menu strip in any HP snapshot.

The strip is 7 rows tall (rows 73..79 in a 131x80 LCD). The interior is rows
74..78 (5 rows). Each cell is 21 or 22 wide. Labels are: EDIT|CURS|BIG ▪|EVAL|FACTO|SIMP.
"""
import os, sys

HP = "/tmp/eqw_test/hp"

def read_pgm(p):
    with open(p, 'rb') as f:
        f.readline(); line = f.readline()
        while line.startswith(b'#'): line = f.readline()
        w, h = map(int, line.split()); f.readline()
        return w, h, f.read(w * h)

def find_snap(name):
    for f in os.listdir(HP):
        if f.endswith(f"_{name}.pgm"):
            return os.path.join(HP, f)
    return None

# Use the boot snapshot (or any with stable menu)
p = find_snap("num_5") or find_snap("num_1")
w, h, data = read_pgm(p)

# Determine menu y band: find the topmost row that is a full black line (in
# bottom 9 rows).
menu_top = None
for y in range(h - 9, h):
    dark = sum(1 for x in range(w) if data[y * w + x] < 0x80)
    if dark > w * 0.9:
        menu_top = y
        break

print(f"menu_top = {menu_top}, h = {h}", file=sys.stderr)

# interior y band:
y0 = menu_top + 1
y1 = h - 1  # bottom border at h-1

# Cell columns: vertical separators
sep_cols = []
for x in range(w):
    dark = sum(1 for y in range(y0, y1) if data[y * w + x] < 0x80)
    if dark >= (y1 - y0):  # full vertical column = separator
        sep_cols.append(x)

print(f"separator cols: {sep_cols}", file=sys.stderr)

# Group consecutive separator columns into single separators
groups = []
prev = None; group = []
for c in sep_cols:
    if prev is None or c - prev > 1:
        if group: groups.append(group)
        group = [c]
    else:
        group.append(c)
    prev = c
if group: groups.append(group)
print(f"separator groups: {groups}", file=sys.stderr)

# 7 separators: left edge, after EDIT, after CURS, after BIG, after EVAL, after FACTO, right edge.
# So 6 cells between them.
if len(groups) < 7:
    # Some labels touch the separator. Fall back to known cell positions.
    # HP cells start at columns 0, 22, 44, 66, 88, 110 (each 22 wide).
    cell_xs = [0, 22, 44, 66, 88, 110, 131]
else:
    cell_xs = [g[0] for g in groups]
    if len(cell_xs) > 7: cell_xs = cell_xs[:7]
print(f"cell xs: {cell_xs}", file=sys.stderr)

# The labels in each cell:
labels = ["EDIT", "CURS", "BIG\xb7", "EVAL", "FACTO", "SIMP"]
# (BIG has a centered dot ▪ next to it)

# For each cell, extract the bbox of dark pixels and split into chars
glyphs = {}

def split_glyphs_in_cell(x_lo, x_hi, y_lo, y_hi):
    cd = []
    for x in range(x_lo + 1, x_hi):  # skip cell left border
        dark = any(data[y * w + x] < 0x80 for y in range(y_lo, y_hi))
        cd.append(dark)
    spans = []; in_g = False; sx = None
    for i, c in enumerate(cd):
        if c and not in_g:
            in_g = True; sx = x_lo + 1 + i
        elif not c and in_g:
            in_g = False; spans.append((sx, x_lo + 1 + i))
    if in_g: spans.append((sx, x_hi))
    return spans

for ci in range(len(labels)):
    label = labels[ci]
    cx_lo = cell_xs[ci]
    cx_hi = cell_xs[ci + 1] if ci + 1 < len(cell_xs) else w
    spans = split_glyphs_in_cell(cx_lo, cx_hi, y0, y1)
    if len(spans) != len(label):
        print(f"cell {ci} '{label}': got {len(spans)} spans (expected {len(label)})", file=sys.stderr)
        continue
    # Find the actual glyph y bounds (skip blank top/bottom rows)
    # Use whole label's pixel y bounds
    yvals = [y for y in range(y0, y1) for x in range(cx_lo, cx_hi) if data[y * w + x] < 0x80]
    if yvals:
        gy0 = min(yvals); gy1 = max(yvals) + 1
    else:
        gy0 = y0; gy1 = y1
    for (sx, ex), c in zip(spans, label):
        rows = []
        for y in range(gy0, gy1):
            bits = 0
            for x in range(sx, ex):
                if data[y * w + x] < 0x80:
                    bits |= (1 << (x - sx))
            rows.append(bits)
        if c not in glyphs:
            glyphs[c] = (ex - sx, gy1 - gy0, rows)

# Print preview
for c in sorted(glyphs.keys()):
    gw, gh, rows = glyphs[c]
    print(f"/* mini '{c}' ({gw}x{gh}) */", file=sys.stderr)
    for r in rows:
        print("  " + "".join("#" if r & (1 << b) else "." for b in range(gw)), file=sys.stderr)

# Pad to 5 rows (HP mini font is 5 tall)
for c in list(glyphs):
    gw, gh, rows = glyphs[c]
    if gh < 5: rows = rows + [0] * (5 - gh)
    elif gh > 5: rows = rows[:5]
    glyphs[c] = (gw, 5, rows)

def safe(c):
    nm = {"·":"cdot",".":"period","+":"plus","-":"minus",
          "(":"lparen",")":"rparen","=":"eq",",":"comma",
          "|":"pipe","/":"slash","\xb7":"cdot"}
    return nm.get(c, c if c.isalnum() else f"u{ord(c):04x}")

print('/* hp_mini.c — auto-generated mini font from HP menu strip */')
print('#include <stdint.h>')
print('#include "hp_font.h"')
print()
for c in sorted(glyphs.keys()):
    gw, gh, rows = glyphs[c]
    print(f"static const uint8_t hp_m_{safe(c)}[5] = {{ {', '.join(str(r) for r in rows)} }};")
print()
print('/* Mini glyph table: char -> {w, advance, rows*}. */')
print('const hp_glyph_t hp_mini_table[256] = {')
for c in sorted(glyphs.keys()):
    if len(c) != 1 or ord(c) > 127: continue
    gw, gh, rows = glyphs[c]
    advance = gw + 1
    if c == "'": entry = "[39]"
    elif c == "\\": entry = "[92]"
    else: entry = f"['{c}']"
    print(f"  {entry} = {{ {gw}, {advance}, hp_m_{safe(c)} }},")
print('};')
print(f'\n/* {len(glyphs)} mini glyphs */')
