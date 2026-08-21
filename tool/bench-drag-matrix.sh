#!/usr/bin/env bash
# Drag-cost matrix: one --bench-drag run per (dragged panel, visible set).
# Writes to an isolated support dir so the listener's settings are untouched.
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
export XDG_DATA_HOME="${XDG_DATA_HOME:-/tmp/tramp-bench-xdg}"
mkdir -p "$XDG_DATA_HOME"
MOVES="${MOVES:-40}"

run() {
  local panel="$1" visible="$2"
  printf '=== drag=%-9s visible=[%s]\n' "$panel" "$visible"
  timeout 300 "$ROOT/build/tramp" --bench-drag "$panel" --bench-moves "$MOVES" \
    --bench-visible "$visible" 2>/dev/null | grep 'bench:'
}

run main ""
run main "playlist"
run main "eq,playlist,settings,about"
run eq "eq"
run playlist "playlist"
run settings "settings"
run about "about"

printf '=== resize=playlist\n'
timeout 300 "$ROOT/build/tramp" --bench-resize --bench-moves "$MOVES" \
  --bench-visible "playlist" 2>/dev/null | grep 'bench:'
