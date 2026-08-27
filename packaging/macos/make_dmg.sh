#!/usr/bin/env bash
# Build a UDZO DMG containing aoide.app (drag-to-Applications).
# The Qt macOS host is not built yet; this script is the packaging shape for when it is.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
# shellcheck disable=SC1090
eval "$(bash "$ROOT/tool/version.sh")"
APP="${AOIDE_MAC_APP:-$ROOT/build/macos/aoide.app}"
STAGE="$ROOT/build/macos/dmg"
OUT="$ROOT/build/macos/Aoide-${version}-macos-universal.dmg"

if [[ ! -d "$APP" ]]; then
  echo "make_dmg: missing $APP (Qt macOS host is not in this tree yet)" >&2
  exit 1
fi

rm -rf "$STAGE"
mkdir -p "$STAGE"
cp -a "$APP" "$STAGE/"
ICNS="$ROOT/packaging/macos/aoide.icns"
if [[ -f "$ICNS" ]]; then
  mkdir -p "$STAGE/$(basename "$APP")/Contents/Resources"
  cp -f "$ICNS" "$STAGE/$(basename "$APP")/Contents/Resources/aoide.icns"
fi
ln -s /Applications "$STAGE/Applications"
cp -f "$ROOT/LICENSE" "$STAGE/"
cp -f "$ROOT/THIRD-PARTY-NOTICES.md" "$STAGE/"

rm -f "$OUT"
hdiutil create -volname "Aoide" -srcfolder "$STAGE" -ov -format UDZO "$OUT"
echo "Wrote $OUT"
