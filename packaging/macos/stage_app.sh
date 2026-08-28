#!/usr/bin/env bash
# Deploy Qt and libmpv into Aoide.app so the DMG is self-contained.
# windeployqt / stage_bundle.sh are the other two platforms; this is the Mac one.
# A missing macdeployqt or a bundle that still needs Homebrew is a hard error —
# the same class of defect stage.ps1 throws on, and the one a Mac without those
# libraries would discover only after download.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD="${AOIDE_BUILD_DIR:-}"
DEST="${AOIDE_BUNDLE_DIR:-$ROOT/build/macos/stage}"

if [[ -z "$BUILD" ]]; then
  if [[ -d "$ROOT/build/macos" ]]; then
    BUILD="$ROOT/build/macos"
  else
    BUILD="$ROOT/build"
  fi
fi

find_built_app() {
  local c
  for c in \
      "${AOIDE_MAC_APP:-}" \
      "$BUILD/Aoide.app" \
      "$ROOT/build/macos/Aoide.app" \
      "$ROOT/build/Aoide.app"; do
    [[ -n "$c" ]] || continue
    if [[ -x "$c/Contents/MacOS/Aoide" ]]; then
      printf '%s\n' "$c"
      return 0
    fi
  done
  return 1
}

if ! built="$(find_built_app)"; then
  echo "stage_app: missing Aoide.app under $BUILD — cmake --build first" >&2
  exit 1
fi

APP="$DEST/Aoide.app"
rm -rf "$DEST"
mkdir -p "$DEST"

# Prefer cmake --install so the staged tree matches BUNDLE DESTINATION. Fall
# back to copying the build-tree app if the install prefix layout is unexpected
# (an older cache, or a prefix that nested the bundle).
if cmake --install "$BUILD" --prefix "$DEST" >/dev/null && \
    [[ -x "$DEST/Aoide.app/Contents/MacOS/Aoide" ]]; then
  :
else
  rm -rf "$APP"
  cp -a "$built" "$APP"
fi

if [[ ! -x "$APP/Contents/MacOS/Aoide" ]]; then
  echo "stage_app: $APP is not a complete Aoide.app" >&2
  exit 1
fi

find_macdeployqt() {
  if command -v macdeployqt >/dev/null; then
    command -v macdeployqt
    return 0
  fi
  if [[ -n "${QT:-}" && -x "$QT/bin/macdeployqt" ]]; then
    printf '%s\n' "$QT/bin/macdeployqt"
    return 0
  fi
  if [[ -n "${QTDIR:-}" && -x "$QTDIR/bin/macdeployqt" ]]; then
    printf '%s\n' "$QTDIR/bin/macdeployqt"
    return 0
  fi
  local qt_core prefix
  qt_core="$(otool -L "$APP/Contents/MacOS/Aoide" | awk '/QtCore\.framework/ { print $1; exit }')"
  if [[ "$qt_core" == /* ]]; then
    # PREFIX/lib/QtCore.framework/Versions/A/QtCore
    prefix="$(cd "$(dirname "$qt_core")/../../../.." && pwd)"
    if [[ -x "$prefix/bin/macdeployqt" ]]; then
      printf '%s\n' "$prefix/bin/macdeployqt"
      return 0
    fi
  fi
  local g
  for g in "$ROOT/.local/qt/"*/macos/bin/macdeployqt; do
    if [[ -x "$g" ]]; then
      printf '%s\n' "$g"
      return 0
    fi
  done
  return 1
}

if ! MACDEPLOYQT="$(find_macdeployqt)"; then
  echo "stage_app: macdeployqt not found; refusing to ship Aoide.app without Qt" >&2
  exit 1
fi

copy_xcframeworks() {
  local dest="$APP/Contents/Frameworks"
  mkdir -p "$dest"
  local found=0 xc fw
  shopt -s nullglob
  for xc in \
      "$ROOT/third_party/libmpv/macos/universal"/*.xcframework \
      "$ROOT/third_party/libmpv/macos/universal"/*/*.xcframework; do
    [[ -d "$xc" ]] || continue
    for fw in "$xc"/macos-*/*.framework; do
      [[ -d "$fw" ]] || continue
      rm -rf "$dest/$(basename "$fw")"
      cp -a "$fw" "$dest/"
      found=1
    done
  done
  shopt -u nullglob
  ((found))
}

# Copy the shape the binary actually linked: a Homebrew build names
# libmpv*.dylib, the xcframework build names Mpv.framework. Shipping the
# other one leaves the loader looking for a path that is not in the bundle.
EXE="$APP/Contents/MacOS/Aoide"
links_mpv_framework() {
  otool -L "$EXE" | grep -q 'Mpv.framework'
}
links_mpv_dylib() {
  otool -L "$EXE" | grep -q 'libmpv'
}
bundle_has_mpv_framework() {
  [[ -e "$APP/Contents/Frameworks/Mpv.framework/Versions/A/Mpv" ]]
}
bundle_has_mpv_dylib() {
  compgen -G "$APP/Contents/Frameworks/libmpv*.dylib" >/dev/null
}

if links_mpv_framework; then
  if ! bundle_has_mpv_framework && ! copy_xcframeworks; then
    echo "stage_app: binary links Mpv.framework but none is in the bundle" >&2
    echo "  run ./tool/fetch_full_libmpv.sh" >&2
    exit 1
  fi
elif links_mpv_dylib; then
  if ! bundle_has_mpv_dylib; then
    if ! command -v pkg-config >/dev/null || ! pkg-config --exists mpv; then
      echo "stage_app: binary links libmpv but pkg-config cannot find it" >&2
      exit 1
    fi
    mkdir -p "$APP/Contents/Frameworks"
    libdir="$(pkg-config --variable=libdir mpv)"
    shopt -s nullglob
    local_mpv=("$libdir"/libmpv*.dylib)
    shopt -u nullglob
    if ((${#local_mpv[@]} == 0)); then
      echo "stage_app: pkg-config found mpv but $libdir has no libmpv*.dylib" >&2
      exit 1
    fi
    cp -a "${local_mpv[@]}" "$APP/Contents/Frameworks/"
  fi
else
  echo "stage_app: $EXE does not link libmpv or Mpv.framework" >&2
  exit 1
fi

# -libpath lets macdeployqt see Homebrew / staged libmpv when the binary
# linked against a path outside the bundle. Do not pass -dmg: make_dmg.sh
# owns the disk image, and a second copy would not be the signed one.
libpaths=()
if [[ -d "$APP/Contents/Frameworks" ]]; then
  libpaths+=(-libpath="$APP/Contents/Frameworks")
fi
if command -v pkg-config >/dev/null && pkg-config --exists mpv; then
  libpaths+=(-libpath="$(pkg-config --variable=libdir mpv)")
fi

"$MACDEPLOYQT" "$APP" -verbose=1 -always-overwrite "${libpaths[@]}"

# qoffscreen is not a dependency macdeployqt can see: it is what a headless
# smoke test runs the staged tree under, and that test clears QT_PLUGIN_PATH so
# a stage missing its plugins cannot borrow the runner's. Same reason
# stage.ps1 / stage_bundle.sh take the offscreen plugin on the other hosts.
# install-qt-action exports QT_ROOT_DIR, not QT or QTDIR, so keying only on
# those skipped this copy on every CI run. macdeployqt's own prefix is the
# reliable answer: it lives at PREFIX/bin, beside PREFIX/plugins.
qt_plugins=""
for qt_cand in \
    "${QT:-}" \
    "${QTDIR:-}" \
    "${QT_ROOT_DIR:-}" \
    "$(cd "$(dirname "$MACDEPLOYQT")/.." && pwd)"; do
  [[ -n "$qt_cand" && -d "$qt_cand/plugins/platforms" ]] || continue
  qt_plugins="$qt_cand/plugins"
  break
done
if [[ -n "$qt_plugins" && -f "$qt_plugins/platforms/libqoffscreen.dylib" ]]; then
  mkdir -p "$APP/Contents/PlugIns/platforms"
  cp -f "$qt_plugins/platforms/libqoffscreen.dylib" \
    "$APP/Contents/PlugIns/platforms/"
fi

if [[ ! -d "$APP/Contents/Frameworks/QtCore.framework" ]]; then
  echo "stage_app: macdeployqt did not deploy QtCore.framework" >&2
  exit 1
fi
if [[ ! -f "$APP/Contents/PlugIns/platforms/libqcocoa.dylib" ]]; then
  echo "stage_app: macdeployqt did not deploy platforms/libqcocoa.dylib" >&2
  exit 1
fi
if links_mpv_framework && ! bundle_has_mpv_framework; then
  echo "stage_app: Mpv.framework is still missing after deploy" >&2
  exit 1
fi
if links_mpv_dylib && ! bundle_has_mpv_dylib; then
  echo "stage_app: libmpv*.dylib is still missing after deploy" >&2
  exit 1
fi
if [[ ! -f "$APP/Contents/Resources/aoide.icns" ]]; then
  echo "stage_app: $APP is missing Contents/Resources/aoide.icns (CMake must install it)" >&2
  exit 1
fi
if [[ ! -d "$APP/Contents/Resources/assets" || ! -d "$APP/Contents/Resources/skins" ]]; then
  echo "stage_app: $APP is missing Contents/Resources/assets or skins" >&2
  exit 1
fi

echo "Staged $APP"
echo "  macdeployqt $MACDEPLOYQT"
