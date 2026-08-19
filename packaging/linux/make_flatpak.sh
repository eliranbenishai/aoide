#!/usr/bin/env bash
# Build a local Flatpak bundle from the staged Qt Linux tree.
# Flathub listing is still a human PR; this artifact is for testing the
# same finish-args the listing will use.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
# shellcheck disable=SC1090
eval "$(bash "$ROOT/tool/version.sh")"
BUNDLE="${TRAMP_BUNDLE_DIR:-$ROOT/build/linux/bundle}"
STATE="$ROOT/build/linux/flatpak-builder"
OUT="$ROOT/build/linux/Tramp-${version}-linux-x86_64.flatpak"

if [[ ! -x "$BUNDLE/tramp" ]]; then
  echo "make_flatpak: missing $BUNDLE/tramp — run packaging/linux/stage_bundle.sh" >&2
  exit 1
fi

if ! command -v flatpak-builder >/dev/null; then
  echo "make_flatpak: flatpak-builder not installed" >&2
  exit 1
fi

export TRAMP_FLATPAK_BUNDLE="$BUNDLE"
flatpak-builder --force-clean --repo="$STATE/repo" "$STATE/build" \
  "$ROOT/packaging/flatpak/com.proximamagnifica.tramp.yml"
flatpak build-bundle "$STATE/repo" "$OUT" com.proximamagnifica.tramp
echo "Wrote $OUT"
