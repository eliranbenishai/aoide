#!/usr/bin/env bash
# Ensure linux/flutter/ephemeral Flutter Linux artifacts exist (Distrobox flake).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
EPHEMERAL="$ROOT/linux/flutter/ephemeral"
DEST="$EPHEMERAL/flutter_linux"
HDR="$DEST/flutter_linux.h"
FLUTTER_ROOT="${FLUTTER_ROOT:-$HOME/flutter}"
ENGINE_ROOT="$FLUTTER_ROOT/bin/cache/artifacts/engine"
SRC="$ENGINE_ROOT/linux-x64/flutter_linux"
MODE="${1:-debug}"

case "$MODE" in
  release|Release) VARIANT=linux-x64-release ;;
  profile|Profile) VARIANT=linux-x64-profile ;;
  *) VARIANT=linux-x64 ;;
esac

seeded=0

if [[ ! -f "$HDR" ]]; then
  if [[ ! -f "$SRC/flutter_linux.h" ]]; then
    echo "ensure_linux_ephemeral: missing engine headers at $SRC" >&2
    exit 1
  fi
  mkdir -p "$DEST"
  cp -a "$SRC"/. "$DEST"/
  echo "ensure_linux_ephemeral: seeded flutter_linux headers"
  seeded=1
fi

SO_DST="$EPHEMERAL/libflutter_linux_gtk.so"
SO_SRC="$ENGINE_ROOT/$VARIANT/libflutter_linux_gtk.so"
if [[ ! -f "$SO_DST" ]]; then
  if [[ ! -f "$SO_SRC" ]]; then
    echo "ensure_linux_ephemeral: missing $SO_SRC" >&2
    exit 1
  fi
  mkdir -p "$EPHEMERAL"
  cp -f "$SO_SRC" "$SO_DST"
  echo "ensure_linux_ephemeral: seeded libflutter_linux_gtk.so ($VARIANT)"
  seeded=1
fi

ICU_DST="$EPHEMERAL/icudtl.dat"
ICU_SRC="$ENGINE_ROOT/linux-x64/icudtl.dat"
if [[ ! -f "$ICU_DST" && -f "$ICU_SRC" ]]; then
  cp -f "$ICU_SRC" "$ICU_DST"
  echo "ensure_linux_ephemeral: seeded icudtl.dat"
  seeded=1
fi

if [[ "$seeded" -eq 0 ]]; then
  exit 0
fi
