#!/usr/bin/env bash
# Build a local Flatpak bundle from the Flutter Linux release tree.
# Flathub listing is still a human PR; this artifact is for testing the
# same finish-args the listing will use.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
# shellcheck disable=SC1090
eval "$(bash "$ROOT/tool/pubspec_version.sh")"
BUNDLE="$ROOT/build/linux/x64/release/bundle"
STATE="$ROOT/build/linux/flatpak-builder"
OUT="$ROOT/build/linux/Tramp-${version}-linux-x86_64.flatpak"

if [[ ! -x "$BUNDLE/tramp" ]]; then
  echo "make_flatpak: missing $BUNDLE/tramp" >&2
  exit 1
fi

if ! command -v flatpak-builder >/dev/null; then
  echo "make_flatpak: flatpak-builder not installed" >&2
  exit 1
fi

export TRAMP_FLATPAK_BUNDLE="$BUNDLE"
flatpak-builder --force-clean --repo="$STATE/repo" "$STATE/build" \
  "$ROOT/packaging/flatpak/com.tramp.tramp.yml"
flatpak build-bundle "$STATE/repo" "$OUT" com.tramp.tramp
echo "Wrote $OUT"
