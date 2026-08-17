#!/usr/bin/env bash
# Install the Qt tramp binary plus assets/skins/desktop into build/linux/bundle.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD="${TRAMP_BUILD_DIR:-$ROOT/build}"
DEST="${TRAMP_BUNDLE_DIR:-$ROOT/build/linux/bundle}"

if [[ ! -x "$BUILD/tramp" ]]; then
  echo "stage_bundle: missing $BUILD/tramp — build the Qt host first" >&2
  exit 1
fi

cmake --install "$BUILD" --prefix "$DEST"
echo "Staged $DEST/tramp"
