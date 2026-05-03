#!/usr/bin/env bash
# Run the demo_step x50ng script in a fresh datadir, emit PGMs to /tmp/demo_step/hp.
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SCRIPTS="$ROOT/scripts"
X50NG="$ROOT/../x50ng"
SRC_DIST="$X50NG/dist"
OUT="/tmp/demo_step/hp"
WORK="/tmp/demo_step/x50_work"

mkdir -p "$OUT"
rm -f "$OUT"/*.pgm
rm -rf "$WORK"
mkdir -p "$WORK/firmware"

ln -sf "$SRC_DIST/firmware/boot-50g.bin" "$WORK/firmware/boot-50g.bin"
ln -sf "$SRC_DIST/firmware/hp4950v215" "$WORK/firmware/hp4950v215"
cp "$SRC_DIST/state.clean"        "$WORK/state"
cp "$SRC_DIST/sram.clean"         "$WORK/sram"
cp "$SRC_DIST/flash.clean"        "$WORK/flash"
cp "$SRC_DIST/s3c2410-sram.clean" "$WORK/s3c2410-sram"
cp "$SRC_DIST/config.lua"         "$WORK/config.lua"
for css in "$SRC_DIST"/style-*.css; do
    ln -sf "$css" "$WORK/$(basename "$css")"
done

X50NG_SCRIPT="$SCRIPTS/demo_step_x50ng.script" \
X50NG_SNAP_PREFIX="$OUT/h" \
"$X50NG/dist/x50ng" \
    --datadir="$WORK" \
    --bootloader="$WORK/firmware/boot-50g.bin" \
    --firmware="$WORK/firmware/hp4950v215/2MB_FIX/2MB_215f.bin" \
    --tui --reset > /tmp/demo_step/x50_log.txt 2>&1
echo "x50ng done. snapshots: $(ls "$OUT" | wc -l)"
ls "$OUT" | head -20
