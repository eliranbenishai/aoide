#!/usr/bin/env bash
# Ensure linux/flutter/ephemeral/flutter_linux headers exist (Distrobox flake).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
EPHEMERAL="$ROOT/linux/flutter/ephemeral"
DEST="$EPHEMERAL/flutter_linux"
HDR="$DEST/flutter_linux.h"
FLUTTER_ROOT="${FLUTTER_ROOT:-$HOME/flutter}"
SRC="$FLUTTER_ROOT/bin/cache/artifacts/engine/linux-x64/flutter_linux"

if [[ -f "$HDR" ]]; then
  exit 0
fi

if [[ ! -f "$SRC/flutter_linux.h" ]]; then
  echo "ensure_linux_ephemeral: missing engine headers at $SRC" >&2
  exit 1
fi

mkdir -p "$DEST"
cp -a "$SRC"/. "$DEST"/
echo "ensure_linux_ephemeral: seeded flutter_linux headers"
