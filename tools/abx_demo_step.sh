#!/usr/bin/env bash
# abx_demo_step.sh — compose side-by-side comparison images for each step.
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
HP="/tmp/demo_step/hp"
SDL="/tmp/demo_step/sdl"
OUT="$ROOT/abx/demo_step"
mkdir -p "$OUT"

for step_pgm in "$HP"/h_*demo_arith_*.pgm; do
    base=$(basename "$step_pgm" .pgm)
    label=$(echo "$base" | sed -E 's/^h_[0-9]+_//')
    sdl_pgm=$(ls "$SDL"/s_*_${label}.pgm 2>/dev/null | head -1)
    if [ -z "$sdl_pgm" ]; then continue; fi
    hp_png="/tmp/_a.png"; sdl_png="/tmp/_b.png"
    convert "$step_pgm" -scale 524x320 "$hp_png"
    convert "$sdl_pgm"  -scale 524x320 "$sdl_png"
    convert "$hp_png" -bordercolor "#444" -border 1x1 -gravity north \
        -background "#222" -fill white -pointsize 14 -font DejaVu-Sans-Mono \
        label:"HP\\ x50ng" +swap -append "/tmp/_al.png"
    convert "$sdl_png" -bordercolor "#444" -border 1x1 -gravity north \
        -background "#222" -fill white -pointsize 14 -font DejaVu-Sans-Mono \
        label:"sdleqw" +swap -append "/tmp/_bl.png"
    convert "/tmp/_al.png" "/tmp/_bl.png" +append "$OUT/${label}.png"
done
echo "wrote $(ls "$OUT" | wc -l) comparisons to $OUT"
