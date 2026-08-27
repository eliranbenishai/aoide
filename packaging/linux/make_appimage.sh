#!/usr/bin/env bash
# Wrap the staged Qt Linux bundle as an x86_64 AppImage.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
# shellcheck disable=SC1090
eval "$(bash "$ROOT/tool/version.sh")"
BUNDLE="${AOIDE_BUNDLE_DIR:-$ROOT/build/linux/bundle}"
APPDIR="$ROOT/build/linux/AppDir"
OUT="$ROOT/build/linux/Aoide-${version}-linux-x86_64.AppImage"

if [[ ! -x "$BUNDLE/aoide" ]]; then
  echo "make_appimage: missing $BUNDLE/aoide — run packaging/linux/stage_bundle.sh" >&2
  exit 1
fi

rm -rf "$APPDIR"
mkdir -p "$APPDIR"
cp -a "$BUNDLE"/. "$APPDIR/"
cp -f "$ROOT/LICENSE" "$APPDIR/"
cp -f "$ROOT/THIRD-PARTY-NOTICES.md" "$APPDIR/"

if [[ -f "$APPDIR/share/applications/com.proximamagnifica.aoide.desktop" ]]; then
  cp "$APPDIR/share/applications/com.proximamagnifica.aoide.desktop" "$APPDIR/com.proximamagnifica.aoide.desktop"
else
  cp "$ROOT/packaging/linux/com.proximamagnifica.aoide.desktop" "$APPDIR/com.proximamagnifica.aoide.desktop"
fi
sed -i 's|^Exec=.*|Exec=aoide %F|' "$APPDIR/com.proximamagnifica.aoide.desktop"
sed -i 's|^Icon=.*|Icon=com.proximamagnifica.aoide|' "$APPDIR/com.proximamagnifica.aoide.desktop"

icon_src="$ROOT/packaging/linux/icons/hicolor/256x256/apps/com.proximamagnifica.aoide.png"
if [[ -f "$icon_src" ]]; then
  cp "$icon_src" "$APPDIR/com.proximamagnifica.aoide.png"
fi

cat >"$APPDIR/AppRun" <<'EOF'
#!/bin/sh
HERE="$(dirname "$(readlink -f "$0")")"
export LD_LIBRARY_PATH="$HERE/lib:${LD_LIBRARY_PATH:-}"
exec "$HERE/aoide" "$@"
EOF
chmod +x "$APPDIR/AppRun"

WORKDIR="$ROOT/build/linux/appimage-tools"
mkdir -p "$WORKDIR"
cd "$WORKDIR"
if [[ ! -x appimagetool ]]; then
  curl -fsSL -o appimagetool.AppImage \
    https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage
  chmod +x appimagetool.AppImage
  ./appimagetool.AppImage --appimage-extract >/dev/null
  ln -sfn squashfs-root/AppRun appimagetool
fi

export ARCH=x86_64
rm -f "$OUT"
./appimagetool "$APPDIR" "$OUT"
chmod +x "$OUT"
echo "Wrote $OUT"
