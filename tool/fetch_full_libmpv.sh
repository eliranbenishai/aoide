#!/usr/bin/env bash
# Download / stage full libmpv for macOS or Linux per third_party/libmpv/pins.json.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PINS="$ROOT/third_party/libmpv/pins.json"
CACHE="$ROOT/third_party/libmpv/.cache"
mkdir -p "$CACHE"

os="$(uname -s)"
case "$os" in
  Darwin)
    OUT="$ROOT/third_party/libmpv/macos/universal"
    mkdir -p "$OUT"
    URL="$(python3 - <<'PY' "$PINS"
import json,sys
print(json.load(open(sys.argv[1]))["macos"]["universal"]["url"])
PY
)"
    ARCHIVE="$CACHE/libmpv-macos-audio-full.tar.gz"
    echo "Downloading $URL"
    curl -L --fail -o "$ARCHIVE" "$URL"
    rm -rf "$OUT"/*
    tar -xvf "$ARCHIVE" -C "$OUT"
    echo "Staged macOS full frameworks under $OUT"
    ;;
  Linux)
    OUT="$ROOT/third_party/libmpv/linux/x86_64"
    mkdir -p "$OUT"
    if compgen -G "$OUT/libmpv.so*" > /dev/null; then
      echo "Found bundled libs in $OUT"
    else
      echo "Linux: install a full distro libmpv (with lavfi filters), or place libmpv.so* in:"
      echo "  $OUT"
      if command -v pkg-config >/dev/null && pkg-config --exists mpv; then
        echo "System mpv: $(pkg-config --modversion mpv) at $(pkg-config --variable=libdir mpv)"
      fi
    fi
    ;;
  *)
    echo "Unsupported OS: $os (use tool/fetch_full_libmpv.ps1 on Windows)" >&2
    exit 1
    ;;
esac
