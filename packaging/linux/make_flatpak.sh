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
if ! command -v eu-strip >/dev/null; then
  echo "make_flatpak: eu-strip not found (install elfutils)" >&2
  exit 1
fi

# The manifest has to name its runtime literally, because it is also the file a
# human PRs to Flathub. That makes it the one place the Qt pin is written twice,
# so check it here rather than letting a Qt bump be discovered as a
# flatpak-builder error about a runtime nobody installed.
MANIFEST="$ROOT/packaging/flatpak/com.proximamagnifica.tramp.yml"
qt_runtime="$(tr -d '\r[:space:]' < "$ROOT/QT_VERSION")"
qt_runtime="${qt_runtime%.*}"
manifest_runtime="$(sed -n 's/^runtime-version:[[:space:]]*"\{0,1\}\([0-9.]*\)"\{0,1\}[[:space:]]*$/\1/p' "$MANIFEST")"
if [[ "$manifest_runtime" != "$qt_runtime" ]]; then
  echo "make_flatpak: $MANIFEST says runtime-version '$manifest_runtime'," \
       "but QT_VERSION pins '$qt_runtime'" >&2
  exit 1
fi

TRAMP_BUNDLE_DIR="$BUNDLE" "$ROOT/packaging/linux/stage_bundle.sh" --no-qt

# stable, not flatpak-builder's default of master. The branch is part of the ref
# a listener sees on install, and `master` reads as a development snapshot; it is
# also the name Flathub publishes under. build-bundle has to be told the same
# thing, because its own default is master too.
BRANCH=stable
flatpak-builder --force-clean --default-branch="$BRANCH" \
  --repo="$STATE/repo" "$STATE/build" \
  "$ROOT/packaging/flatpak/com.proximamagnifica.tramp.yml"
flatpak build-bundle "$STATE/repo" "$OUT" com.proximamagnifica.tramp "$BRANCH"
echo "Wrote $OUT"
