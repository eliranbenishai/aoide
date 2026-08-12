#!/usr/bin/env bash
# Measure wall-clock time from SessionHostApp quit start to process exit.
# Fails if that interval exceeds the budget (default 500ms).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUDGET_MS="${TRAMP_QUIT_BUDGET_MS:-500}"
BUILD="${TRAMP_QUIT_BUILD:-release}"
BUNDLE="$ROOT/build/linux/x64/${BUILD}/bundle"
BIN="$BUNDLE/tramp"

if [[ ! -x "$BIN" ]]; then
  echo "missing $BIN — build with: flutter build linux --${BUILD}" >&2
  exit 2
fi

HOME_DIR="$(mktemp -d)"
cleanup() {
  rm -rf "$HOME_DIR"
}
trap cleanup EXIT

export HOME="$HOME_DIR"
export XDG_CONFIG_HOME="$HOME_DIR/.config"
export XDG_DATA_HOME="$HOME_DIR/.local/share"
export XDG_CACHE_HOME="$HOME_DIR/.cache"
export TRAMP_AUTO_QUIT=1
export LD_LIBRARY_PATH="$BUNDLE/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export LIBMPV_LIBRARY_PATH="${LIBMPV_LIBRARY_PATH:-$BUNDLE/lib/libmpv.so.2}"

LOG="$(mktemp)"
START_NS="$(date +%s%N)"
set +e
timeout --signal=KILL 45s "$BIN" >"$LOG" 2>&1
STATUS=$?
set -e
END_NS="$(date +%s%N)"

BEGIN_LINE="$(grep -E '^TRAMP_QUIT_BEGIN ' "$LOG" || true)"
if [[ -z "$BEGIN_LINE" ]]; then
  echo "FAIL: no TRAMP_QUIT_BEGIN marker (process status=$STATUS)" >&2
  tail -n 80 "$LOG" >&2
  exit 1
fi

BEGIN_US="$(awk '{print $2}' <<<"$BEGIN_LINE")"
END_US=$((END_NS / 1000))
ELAPSED_MS=$(( (END_US - BEGIN_US) / 1000 ))
if [[ "$ELAPSED_MS" -lt 0 ]]; then
  # Clock skew fallback: whole process lifetime.
  ELAPSED_MS=$(( (END_NS - START_NS) / 1000000 ))
fi

echo "quit_ms=$ELAPSED_MS budget_ms=$BUDGET_MS"
echo "--- harness log (quit-related) ---"
grep -E 'TRAMP_QUIT_BEGIN|\[DEBUG-quit\]|RemoveWindow|compositor shaders|g_mutex_clear' "$LOG" || true

if [[ "$ELAPSED_MS" -gt "$BUDGET_MS" ]]; then
  echo "FAIL: quit took ${ELAPSED_MS}ms (budget ${BUDGET_MS}ms)" >&2
  exit 1
fi

echo "PASS"
exit 0
