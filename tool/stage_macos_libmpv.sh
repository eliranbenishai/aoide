#!/usr/bin/env bash
# Replace media_kit slim macOS xcframeworks with the staged audio-full build.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
STAGED="$ROOT/third_party/libmpv/macos/universal"

if [[ ! -d "$STAGED" ]]; then
  echo "stage_macos_libmpv: missing $STAGED — run tool/fetch_full_libmpv.sh first" >&2
  exit 1
fi

mapfile -t frameworks < <(find "$STAGED" -name '*.xcframework' -print)
if [[ ${#frameworks[@]} -eq 0 ]]; then
  echo "stage_macos_libmpv: no .xcframework under $STAGED" >&2
  exit 1
fi

PUB_CACHE="${PUB_CACHE:-${FLUTTER_ROOT:+${FLUTTER_ROOT}/.pub-cache}}"
PUB_CACHE="${PUB_CACHE:-$HOME/.pub-cache}"

dests=()
while IFS= read -r d; do
  dests+=("$d")
done < <(find "$PUB_CACHE" "$ROOT/macos" -type d \( \
    -path '*media_kit_libs_macos_audio*/macos/Frameworks' -o \
    -path '*media_kit_libs_macos_audio*/Frameworks' \
  \) 2>/dev/null || true)

if [[ ${#dests[@]} -eq 0 ]]; then
  echo "stage_macos_libmpv: no media_kit Frameworks dirs (flutter pub get / pod install first)" >&2
  exit 1
fi

for dest in "${dests[@]}"; do
  echo "stage_macos_libmpv: $dest"
  for fw in "${frameworks[@]}"; do
    cp -a "$fw" "$dest/"
  done
done
echo "stage_macos_libmpv: replaced ${#frameworks[@]} xcframework(s) in ${#dests[@]} location(s)"
