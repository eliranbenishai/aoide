#!/usr/bin/env bash
# THROW AWAY — release-build + run the one-engine two-window Linux spike.
# Overwrites build/linux/x64/release/bundle/tramp.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

FLUTTER="${FLUTTER_BIN:-$HOME/flutter/bin/flutter}"

if command -v distrobox >/dev/null 2>&1; then
  distrobox enter flutter-dev -- env -u WAYLAND_DISPLAY -u WAYLAND_SOCKET \
    "$FLUTTER" build linux --release -t lib/windowing_spike.dart
else
  "$FLUTTER" build linux --release -t lib/windowing_spike.dart
fi

BUNDLE="$ROOT/build/linux/x64/release/bundle"
export LD_LIBRARY_PATH="$BUNDLE/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
exec "$BUNDLE/tramp" "$@"
