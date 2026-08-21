#!/usr/bin/env bash
# Build a local Flatpak bundle from a Qt-less staging of the Linux tree.
# Flathub listing is still a human PR; this artifact is for testing the
# same finish-args the listing will use.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
# shellcheck disable=SC1090
eval "$(bash "$ROOT/tool/version.sh")"

# The Flatpak gets its own staging directory because it is the only artifact
# that must not carry Qt: it runs on org.kde.Platform, which exists to provide
# Qt. build/linux/bundle stays exactly as it was — the tarball and the AppImage
# share it and both need the Qt in it. Same script and same code path for all
# three; the only difference is the flag below, and it is passed here, once.
# com.proximamagnifica.tramp.yml names this directory as its source.
BUNDLE="$ROOT/build/linux/flatpak-bundle"
STATE="$ROOT/build/linux/flatpak-builder"
OUT="$ROOT/build/linux/Tramp-${version}-linux-x86_64.flatpak"

if ! command -v flatpak-builder >/dev/null; then
  echo "make_flatpak: flatpak-builder not installed" >&2
  exit 1
fi

TRAMP_BUNDLE_DIR="$BUNDLE" "$ROOT/packaging/linux/stage_bundle.sh" --no-qt

flatpak-builder --force-clean --repo="$STATE/repo" "$STATE/build" \
  "$ROOT/packaging/flatpak/com.proximamagnifica.tramp.yml"
flatpak build-bundle "$STATE/repo" "$OUT" com.proximamagnifica.tramp
echo "Wrote $OUT"
