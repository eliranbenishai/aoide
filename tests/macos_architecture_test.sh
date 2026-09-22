#!/usr/bin/env bash
# Exercise the packaging gates with tiny Mach-O files, without building Aoide.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
source "$ROOT/packaging/macos/architecture.sh"
CHECK_DIR="$(mktemp -d "${TMPDIR:-/tmp}/aoide-macos-architecture.XXXXXX")"
trap 'rm -rf "$CHECK_DIR"' EXIT

fail() { echo "FAIL: $*" >&2; exit 1; }

printf 'int example(void) { return 42; }\n' > "$CHECK_DIR/example.c"
clang -arch arm64 -c "$CHECK_DIR/example.c" -o "$CHECK_DIR/arm64.o"
clang -arch x86_64 -c "$CHECK_DIR/example.c" -o "$CHECK_DIR/intel.o"
lipo -create "$CHECK_DIR/arm64.o" "$CHECK_DIR/intel.o" -output "$CHECK_DIR/universal.o"
APP="$CHECK_DIR/Aoide.app"
FRAMEWORK="$APP/Contents/Frameworks/Example.framework"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources" "$FRAMEWORK/Versions/A"
cp "$CHECK_DIR/arm64.o" "$APP/Contents/MacOS/Aoide"
cp "$CHECK_DIR/universal.o" "$FRAMEWORK/Versions/A/Example"
chmod 751 "$FRAMEWORK/Versions/A/Example"
ln -s A "$FRAMEWORK/Versions/Current"
ln -s Versions/Current/Example "$FRAMEWORK/Example"
if require_arm64_bundle "$APP" 2>"$CHECK_DIR/expected-error"; then
  fail 'universal dependency was accepted'
fi
thin_arm64_bundle_dependencies "$APP"
require_arm64_bundle "$APP"
[[ "$(stat -f '%Lp' "$FRAMEWORK/Versions/A/Example")" == 751 ]]
[[ "$(readlink "$FRAMEWORK/Example")" == Versions/Current/Example ]]
[[ "$(readlink "$FRAMEWORK/Versions/Current")" == A ]]
[[ "$(lipo -archs "$CHECK_DIR/universal.o")" == 'x86_64 arm64' ]]
echo 'PASS: universal dependency thinned; source, mode and symlinks preserved'

cp "$CHECK_DIR/intel.o" "$FRAMEWORK/Versions/A/Example"
if thin_arm64_bundle_dependencies "$APP" >"$CHECK_DIR/expected-error" 2>&1; then
  fail 'Intel-only dependency was accepted'
fi
echo 'PASS: Intel-only dependency rejected'
for executable in intel.o universal.o; do
  cp "$CHECK_DIR/$executable" "$APP/Contents/MacOS/Aoide"
  if thin_arm64_bundle_dependencies "$APP" 2>"$CHECK_DIR/expected-error"; then
    fail "$executable app executable was accepted"
  fi
  cmp "$CHECK_DIR/$executable" "$APP/Contents/MacOS/Aoide"
done
echo 'PASS: Intel and universal app executables rejected without mutation'

# Run copied scripts in a temporary repo so even a broken gate cannot erase a
# real build/macos/dmg directory. Only architecture validation should be reached.
RUN_ROOT="$CHECK_DIR/repo"
mkdir -p "$RUN_ROOT/packaging/macos" "$RUN_ROOT/tool" "$RUN_ROOT/build/macos/dmg"
cp "$ROOT/packaging/macos/"{architecture,stage_app,make_dmg,notarize}.sh "$RUN_ROOT/packaging/macos/"
cp "$ROOT/tool/version.sh" "$RUN_ROOT/tool/"
cp "$ROOT/VERSION" "$RUN_ROOT/"
chmod +x "$APP/Contents/MacOS/Aoide"
touch "$APP/Contents/Info.plist" "$APP/Contents/Resources/aoide.icns"
mkdir -p "$CHECK_DIR/stage"
printf 'preserve stage\n' > "$CHECK_DIR/stage/marker"
printf 'preserve wrapper\n' > "$RUN_ROOT/build/macos/dmg/marker"
printf 'preserve image\n' > "$CHECK_DIR/preserved.dmg"
for script in stage_app make_dmg notarize; do
  if AOIDE_BUILD_DIR="$RUN_ROOT/build" AOIDE_BUNDLE_DIR="$CHECK_DIR/stage" \
      AOIDE_MAC_APP="$APP" AOIDE_MAC_DMG="$CHECK_DIR/preserved.dmg" \
      MACOS_CERTIFICATE_BASE64='' MACOS_CERTIFICATE_PASSWORD='' \
      bash "$RUN_ROOT/packaging/macos/$script.sh" >"$CHECK_DIR/$script.log" 2>&1; then
    fail "$script accepted universal app"
  fi
  [[ "$(cat "$CHECK_DIR/$script.log")" == *'expected arm64 only'* ]] ||
    fail "$script failed before reaching architecture validation"
  [[ "$(cat "$CHECK_DIR/stage/marker")" == 'preserve stage' ]]
  [[ "$(cat "$RUN_ROOT/build/macos/dmg/marker")" == 'preserve wrapper' ]]
  [[ "$(cat "$CHECK_DIR/preserved.dmg")" == 'preserve image' ]]
  cmp "$CHECK_DIR/universal.o" "$APP/Contents/MacOS/Aoide"
done
echo 'PASS: staging, DMG wrapping and notarization refuse universal app before mutation'
