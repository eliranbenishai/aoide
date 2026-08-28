#!/usr/bin/env bash
# Build a UDZO DMG containing Aoide.app (drag-to-Applications).
# The .app must already be complete — CMake seals aoide.icns, and stage_app.sh
# deploys Qt. This script must not mutate the source bundle: notarize.sh signs
# it first, and a post-hoc resource copy would break that seal.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
# shellcheck disable=SC1090
eval "$(bash "$ROOT/tool/version.sh")"

pick_app() {
  local c
  for c in \
      "${AOIDE_MAC_APP:-}" \
      "$ROOT/build/macos/stage/Aoide.app" \
      "$ROOT/build/macos/Aoide.app" \
      "$ROOT/build/Aoide.app"; do
    [[ -n "$c" ]] || continue
    if [[ -x "$c/Contents/MacOS/Aoide" ]]; then
      printf '%s\n' "$c"
      return 0
    fi
  done
  printf '%s\n' "${AOIDE_MAC_APP:-$ROOT/build/macos/stage/Aoide.app}"
  return 1
}

if ! APP="$(pick_app)"; then
  echo "make_dmg: $APP is not a complete Aoide.app (missing Contents/MacOS/Aoide)" >&2
  echo "  cmake --build, then packaging/macos/stage_app.sh" >&2
  exit 1
fi
if [[ ! -f "$APP/Contents/Info.plist" ]]; then
  echo "make_dmg: $APP is missing Contents/Info.plist" >&2
  exit 1
fi
if [[ ! -f "$APP/Contents/Resources/aoide.icns" ]]; then
  echo "make_dmg: $APP is missing Contents/Resources/aoide.icns (CMake must install it; this script will not inject one)" >&2
  exit 1
fi

STAGE="$ROOT/build/macos/dmg"
OUT="${AOIDE_MAC_DMG:-$ROOT/build/macos/Aoide-${version}-macos-universal.dmg}"

rm -rf "$STAGE"
mkdir -p "$STAGE"
cp -a "$APP" "$STAGE/Aoide.app"
ln -s /Applications "$STAGE/Applications"
cp -f "$ROOT/LICENSE" "$STAGE/"
cp -f "$ROOT/THIRD-PARTY-NOTICES.md" "$STAGE/"

mkdir -p "$(dirname "$OUT")"
rm -f "$OUT"
hdiutil create -volname "Aoide" -srcfolder "$STAGE" -ov -format UDZO "$OUT"
echo "Wrote $OUT"
