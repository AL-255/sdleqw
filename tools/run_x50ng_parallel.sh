#!/usr/bin/env bash
# Run x50ng test chunks in parallel. Each chunk gets its own working datadir
# (a copy of dist/firmware + the clean state files) so multiple x50ng
# instances don't fight over the same flash/state.
set -e

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SCRIPTS="$ROOT/scripts"
X50NG="$ROOT/../x50ng"
SRC_DIST="$X50NG/dist"
WORK="/tmp/eqw_test"
SNAPS="$WORK/hp"
RUNS="$WORK/runs"

mkdir -p "$SNAPS" "$RUNS"
rm -f "$SNAPS"/*.pgm "$SNAPS"/*.png

CHUNKS=$(ls "$SCRIPTS"/x50ng_chunk_*.script | wc -l)
echo "Running $CHUNKS chunks in parallel..."

run_chunk() {
    local n="$1"
    local script="$SCRIPTS/x50ng_chunk_${n}.script"
    local dist="$RUNS/dist_${n}"
    rm -rf "$dist"
    mkdir -p "$dist/firmware"
    # Symlink firmware (read-only) to save copy time.
    ln -sf "$SRC_DIST/firmware/boot-50g.bin" "$dist/firmware/boot-50g.bin"
    ln -sf "$SRC_DIST/firmware/hp4950v215" "$dist/firmware/hp4950v215"
    cp "$SRC_DIST/state.clean"        "$dist/state"
    cp "$SRC_DIST/sram.clean"         "$dist/sram"
    cp "$SRC_DIST/flash.clean"        "$dist/flash"
    cp "$SRC_DIST/s3c2410-sram.clean" "$dist/s3c2410-sram"
    cp "$SRC_DIST/config.lua"         "$dist/config.lua"
    # Symlink CSS (irrelevant for TUI)
    for css in "$SRC_DIST"/style-*.css; do
        ln -sf "$css" "$dist/$(basename "$css")"
    done

    X50NG_SCRIPT="$script" \
    X50NG_SNAP_PREFIX="$SNAPS/h${n}" \
    "$X50NG/dist/x50ng" \
        --datadir="$dist" \
        --bootloader="$dist/firmware/boot-50g.bin" \
        --firmware="$dist/firmware/hp4950v215/2MB_FIX/2MB_215f.bin" \
        --tui --reset \
        > "$RUNS/log_${n}.txt" 2>&1
    echo "chunk ${n} done"
}

export -f run_chunk
export ROOT SCRIPTS X50NG SRC_DIST WORK SNAPS RUNS

# Launch all chunks in parallel, throttled to ncpu/2 in flight to avoid
# starvation on slow systems.
PARALLEL=$(( $(nproc) / 2 ))
if [ "$PARALLEL" -lt 4 ]; then PARALLEL=4; fi
if [ "$PARALLEL" -gt $CHUNKS ]; then PARALLEL=$CHUNKS; fi
echo "in-flight cap: $PARALLEL"

i=0
pids=()
for s in "$SCRIPTS"/x50ng_chunk_*.script; do
    n=$(basename "$s" .script | sed 's/x50ng_chunk_//')
    run_chunk "$n" &
    pids+=($!)
    i=$((i+1))
    while [ "$(jobs -rp | wc -l)" -ge "$PARALLEL" ]; do
        wait -n 2>/dev/null || true
    done
done
wait

echo "all chunks done. snapshots: $(ls "$SNAPS" | wc -l)"
