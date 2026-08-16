#!/usr/bin/env bash
# Time windowManager getPosition / setPosition inside a running release tramp.
# No pointer injection. Throwaway HOME.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${TRAMP_DRAG_BUILD:-release}"
BUNDLE="$ROOT/build/linux/x64/${BUILD}/bundle"
BIN="$BUNDLE/tramp"

if [[ ! -x "$BIN" ]]; then
  echo "missing $BIN" >&2
  exit 2
fi

HOME_DIR="$(mktemp -d)"
LOG="$(mktemp)"
cleanup() { rm -rf "$HOME_DIR"; rm -f "$LOG"; }
trap cleanup EXIT

export HOME="$HOME_DIR"
export XDG_CONFIG_HOME="$HOME_DIR/.config"
export XDG_DATA_HOME="$HOME_DIR/.local/share"
export XDG_CACHE_HOME="$HOME_DIR/.cache"
export GDK_BACKEND=x11
unset WAYLAND_DISPLAY WAYLAND_SOCKET
export LD_LIBRARY_PATH="$BUNDLE/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export LIBMPV_LIBRARY_PATH="${LIBMPV_LIBRARY_PATH:-$BUNDLE/lib/libmpv.so.2}"
export TRAMP_POSITION_BENCH=1
export TRAMP_SOLO_MAIN="${TRAMP_SOLO_MAIN:-1}"

echo "flags: SOLO_MAIN=$TRAMP_SOLO_MAIN OPAQUE=${TRAMP_OPAQUE_WINDOWS:-0}"
timeout --signal=KILL 45s "$BIN" >"$LOG" 2>&1 || true
grep -E '^TRAMP_POS_BENCH ' "$LOG" || {
  echo "FAIL: no TRAMP_POS_BENCH lines" >&2
  tail -n 60 "$LOG" >&2
  exit 1
}
