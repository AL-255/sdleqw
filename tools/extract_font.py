#!/usr/bin/env python3
"""extract HP fonts from x50ng snapshots, emit C glyph table."""
import os, sys

HP = "/tmp/eqw_test/hp"

def read_pgm(path):
    with open(path, "rb") as f:
        f.readline()
        line = f.readline()
        while line.startswith(b"#"): line = f.readline()
        w, h = map(int, line.split())
        f.readline()
        data = f.read(w * h)
    return w, h, data

def find_bbox(w, h, data, y0, y1, x0=0, x1=None):
    if x1 is None: x1 = w
    minx, miny, maxx, maxy = w, h, -1, -1
    for y in range(y0, y1):
        for x in range(x0, x1):
            if data[y * w + x] < 0x80:
                minx = min(minx, x); miny = min(miny, y)
                maxx = max(maxx, x); maxy = max(maxy, y)
    return (minx, miny, maxx + 1, maxy + 1) if maxx >= 0 else None

def find_snap(name):
    for f in os.listdir(HP):
        if f.endswith(f"_{name}.pgm"):
            return os.path.join(HP, f)
    return None

def extract_solo(name):
    p = find_snap(name)
    if not p: return None
    w, h, data = read_pgm(p)
    bb = find_bbox(w, h, data, 0, h - 9)
    if not bb: return None
    x0, y0, x1, y1 = bb
    rows = []
    for y in range(y0, y1):
        bits = 0
        for x in range(x0, x1):
            if data[y * w + x] < 0x80:
                bits |= (1 << (x - x0))
        rows.append(bits)
    return (x1 - x0, y1 - y0, rows)

def extract_row(name, expected):
    """Snapshot has expected chars in a row; split by gap columns."""
    p = find_snap(name)
    if not p: return {}
    w, h, data = read_pgm(p)
    bb = find_bbox(w, h, data, 0, h - 9)
    if not bb: return {}
    x0, y0, x1, y1 = bb
    cols = []
    for x in range(x0, x1):
        cols.append(any(data[y * w + x] < 0x80 for y in range(y0, y1)))
    spans = []
    in_g = False; sx = None
    for i, c in enumerate(cols):
        if c and not in_g: in_g = True; sx = x0 + i
        elif not c and in_g:
            in_g = False
            spans.append((sx, x0 + i))
    if in_g: spans.append((sx, x1))
    if len(spans) != len(expected): return {}
    out = {}
    for (sx, ex), c in zip(spans, expected):
        rows = []
        for y in range(y0, y1):
            bits = 0
            for x in range(sx, ex):
                if data[y * w + x] < 0x80:
                    bits |= (1 << (x - sx))
            rows.append(bits)
        out[c] = (ex - sx, y1 - y0, rows)
    return out

glyphs = {}

# Solo glyphs
solo_tests = []
for d in "0123456789":
    solo_tests.append((f"num_{d}", d))
for c in "ABEFGHJKLMNORTYZ":
    solo_tests.append((f"name_{c}", c))
for c in "abefghjklmnortyz":
    solo_tests.append((f"name_{c}", c))
solo_tests.append(("name_X", "X"))
solo_tests.append(("name_x", "x"))
for name, c in solo_tests:
    g = extract_solo(name)
    if g: glyphs[c] = g

# Multi-char extractions for punctuation
for tname, expected in [
    ("num_3p14", "3.14"),
    ("add_1_2",  "1+2"),
    ("sub_5_3",  "5-3"),
    ("mul_2_3",  "2\xb73"),  # · is centered dot
    ("uf_F_x",   "F(X)"),
    ("complex_X_Y", "(X,Y)"),
    ("where_const_eq", "5|X=5"),
]:
    extras = extract_row(tname, list(expected))
    for c, g in extras.items():
        if c not in glyphs:
            glyphs[c] = g

# Print visual preview
for c in sorted(glyphs.keys()):
    gw, gh, rows = glyphs[c]
    print(f"/* '{c}' ({gw}x{gh}) */", file=sys.stderr)
    for r in rows:
        s = "".join("#" if r & (1 << b) else "." for b in range(gw))
        print(f"  {s}", file=sys.stderr)

# Emit C
def safe_name(c):
    nm = {".":"period","+":"plus","-":"minus","*":"star",
          "(":"lparen",")":"rparen","=":"eq",",":"comma","|":"pipe",
          "\xb7":"cdot"}
    if c in nm: return nm[c]
    if c.isalpha() or c.isdigit(): return c
    return f"u{ord(c):04x}"

for c in sorted(glyphs.keys()):
    gw, gh, rows = glyphs[c]
    print(f"static const uint8_t hp_{safe_name(c)}[{gh}] = {{ {', '.join(str(r) for r in rows)} }};")

# Generate the lookup table block, indexed by char code
print()
print("/* HP-extracted glyph table; merge with stack_table[] in font.c */")
print("typedef struct {{ int w, h, advance, baseline, axis; const uint8_t *rows; }} _hp_gent;")
for c in sorted(glyphs.keys()):
    gw, gh, rows = glyphs[c]
    code = ord(c) if len(c) == 1 else 128
    name = safe_name(c)
    print(f"  /* '{c}' (code {code}) */ {{ {gw}, {gh}, {gw + 1}, 5, 3, hp_{name} }},")

print(f"\n/* {len(glyphs)} glyphs */")
