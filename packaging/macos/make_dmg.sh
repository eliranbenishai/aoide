#!/usr/bin/env bash
# Build a UDZO DMG containing tramp.app (drag-to-Applications).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
# shellcheck disable=SC1090
eval "$(bash "$ROOT/tool/pubspec_version.sh")"
APP="$ROOT/build/macos/Build/Products/Release/tramp.app"
STAGE="$ROOT/build/macos/dmg"
OUT="$ROOT/build/macos/Tramp-${version}-macos-universal.dmg"

if [[ ! -d "$APP" ]]; then
  echo "make_dmg: missing $APP" >&2
  exit 1
fi

rm -rf "$STAGE"
mkdir -p "$STAGE"
cp -a "$APP" "$STAGE/"
ln -s /Applications "$STAGE/Applications"
cp -f "$ROOT/LICENSE" "$STAGE/"
cp -f "$ROOT/THIRD-PARTY-NOTICES.md" "$STAGE/"

rm -f "$OUT"
hdiutil create -volname "Tramp" -srcfolder "$STAGE" -ov -format UDZO "$OUT"
echo "Wrote $OUT"
