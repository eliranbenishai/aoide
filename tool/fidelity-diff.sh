#!/usr/bin/env bash
# Mockup-fidelity gate for chrome changes.
#
#   tool/fidelity-diff.sh --baseline   capture the current build as reference
#   tool/fidelity-diff.sh              dump the current build and diff it
#
# Prints per-panel RMSE and peak absolute error, normalised to 0..1, measured on
# the chrome composited over an opaque background plus the alpha channel on its
# own. Comparing the PNGs raw is misleading: colour channels inside fully
# transparent pixels carry arbitrary values and swamp the peak metric.
# Fidelity is mockup-absolute, so any blur/gradient rewrite answers to this.
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
REF="$ROOT/.scratch/fidelity/baseline"
NEW="$ROOT/.scratch/fidelity/current"
DIFF="$ROOT/.scratch/fidelity/diff"
BIN="$ROOT/build/tramp"
THRESHOLD="${THRESHOLD:-0.01}"

dump() {
  local dir="$1"
  rm -rf "$dir"
  mkdir -p "$dir"
  QT_QPA_PLATFORM=offscreen XDG_DATA_HOME=/tmp/tramp-bench-xdg "$BIN" --dump-chrome "$dir" >/dev/null
}

if [[ "${1:-}" == "--baseline" ]]; then
  dump "$REF"
  echo "baseline captured in $REF"
  ls -1 "$REF"
  exit 0
fi

if [[ ! -d "$REF" ]]; then
  echo "no baseline; run: tool/fidelity-diff.sh --baseline (on unmodified code)" >&2
  exit 2
fi

dump "$NEW"
mkdir -p "$DIFF"
status=0
# Walk the current dump first. A state the build has started producing but the
# baseline has never seen is invisible to the comparison below, which only knows
# the names the baseline already holds — so adding a panel to --dump-chrome
# would otherwise slip under the gate and stay unwatched until someone happened
# to reseed. Report it, and fail.
for new in "$NEW"/*.png; do
  [[ -e "$new" ]] || continue
  name="$(basename "$new")"
  if [[ ! -f "$REF/$name" ]]; then
    echo "UNSEEDED $name (no baseline entry; reseed with: tool/fidelity-diff.sh --baseline)"
    status=1
  fi
done
for ref in "$REF"/*.png; do
  [[ -e "$ref" ]] || continue
  name="$(basename "$ref")"
  new="$NEW/$name"
  if [[ ! -f "$new" ]]; then
    echo "MISSING $name"
    status=1
    continue
  fi
  norm='s/.*(\([0-9.e+-]*\)).*/\1/p'
  flatA="$DIFF/.flat-ref-$name"
  flatB="$DIFF/.flat-new-$name"
  magick "$ref" -background gray50 -alpha remove -alpha off "$flatA"
  magick "$new" -background gray50 -alpha remove -alpha off "$flatB"
  rmse="$(magick compare -metric RMSE "$flatA" "$flatB" "$DIFF/$name" 2>&1 >/dev/null | sed -n "$norm")"
  peak="$(magick compare -metric PAE "$flatA" "$flatB" null: 2>&1 >/dev/null | sed -n "$norm")"
  alpha="$(magick compare -metric PAE \
    \( "$ref" -alpha extract \) \( "$new" -alpha extract \) null: 2>&1 >/dev/null | sed -n "$norm")"
  rm -f "$flatA" "$flatB"
  verdict="ok"
  if awk -v r="${rmse:-1}" -v t="$THRESHOLD" 'BEGIN{exit !(r>t)}'; then
    verdict="OVER THRESHOLD"
    status=1
  fi
  printf '%-26s rmse=%-12s peak=%-12s alpha_peak=%-12s %s\n' \
    "$name" "${rmse:-?}" "${peak:-?}" "${alpha:-?}" "$verdict"
done
echo "threshold rmse <= $THRESHOLD ; diff images in $DIFF"
exit $status
