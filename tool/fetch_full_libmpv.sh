#!/usr/bin/env bash
# Download / stage full libmpv for macOS or Linux per third_party/libmpv/pins.json.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PINS="$ROOT/third_party/libmpv/pins.json"
CACHE="$ROOT/third_party/libmpv/.cache"
mkdir -p "$CACHE"

sha256_of() {
  # macOS ships shasum, not GNU sha256sum.
  if command -v sha256sum >/dev/null; then
    sha256sum "$1" | awk '{ print $1 }'
  elif command -v shasum >/dev/null; then
    shasum -a 256 "$1" | awk '{ print $1 }'
  else
    echo "fetch_full_libmpv: need sha256sum or shasum" >&2
    exit 1
  fi
}

sha256_eq() {
  local got expected
  got="$(printf '%s' "$1" | tr '[:upper:]' '[:lower:]')"
  expected="$(printf '%s' "$2" | tr '[:upper:]' '[:lower:]')"
  [[ -n "$expected" && "$expected" != "null" && "$got" == "$expected" ]]
}

os="$(uname -s)"
case "$os" in
  Darwin)
    OUT="$ROOT/third_party/libmpv/macos/universal"
    mkdir -p "$OUT"
    URL="$(python3 - <<'PY' "$PINS"
import json,sys
print(json.load(open(sys.argv[1]))["macos"]["universal"]["url"])
PY
)"
    EXPECTED="$(python3 - <<'PY' "$PINS"
import json,sys
print(json.load(open(sys.argv[1]))["macos"]["universal"].get("archiveSha256") or "")
PY
)"
    ARCHIVE_NAME="$(python3 - <<'PY' "$PINS"
import json,sys
pin=json.load(open(sys.argv[1]))["macos"]["universal"]
print(pin.get("archive") or "libmpv-macos-audio-full.tar.gz")
PY
)"
    ARCHIVE="$CACHE/$ARCHIVE_NAME"
    if [[ ! -f "$ARCHIVE" ]] || ! sha256_eq "$(sha256_of "$ARCHIVE")" "$EXPECTED"; then
      echo "Downloading $URL"
      curl -L --fail -o "$ARCHIVE" "$URL"
    fi
    if [[ -n "$EXPECTED" && "$EXPECTED" != "null" ]]; then
      got="$(sha256_of "$ARCHIVE")"
      if ! sha256_eq "$got" "$EXPECTED"; then
        echo "fetch_full_libmpv: archive SHA-256 mismatch: got $got expected $EXPECTED" >&2
        exit 1
      fi
      echo "Archive OK ($got)"
    fi
    tmp="$(mktemp -d "${TMPDIR:-/tmp}/aoide-libmpv.XXXXXX")"
    trap 'rm -rf "$tmp"' EXIT
    tar -xf "$ARCHIVE" -C "$tmp"
    # The media-kit archive wraps xcframeworks in a versioned directory.
    # Lift them so CMake can look at macos/universal/Mpv.xcframework without
    # baking the pin name into the build.
    src="$tmp"
    shopt -s nullglob
    top=("$tmp"/*)
    shopt -u nullglob
    if ((${#top[@]} == 1)) && [[ -d "${top[0]}" ]] && \
        compgen -G "${top[0]}/*.xcframework" >/dev/null; then
      src="${top[0]}"
    fi
    rm -rf "$OUT"
    mkdir -p "$OUT"
    shopt -s nullglob
    for xc in "$src"/*.xcframework; do
      mv "$xc" "$OUT/"
    done
    shopt -u nullglob
    if [[ ! -d "$OUT/Mpv.xcframework" ]]; then
      echo "fetch_full_libmpv: archive did not contain Mpv.xcframework" >&2
      exit 1
    fi
    mpv_bin=""
    while IFS= read -r -d '' f; do
      mpv_bin="$f"
      break
    done < <(find "$OUT/Mpv.xcframework" -path '*/Versions/A/Mpv' -type f -print0)
    if [[ -z "$mpv_bin" ]]; then
      echo "fetch_full_libmpv: Mpv.xcframework has no Versions/A/Mpv binary" >&2
      exit 1
    fi
    if grep -a -q -- '--disable-filters' "$mpv_bin"; then
      echo "fetch_full_libmpv: staged Mpv looks slim (--disable-filters). Refusing." >&2
      exit 1
    fi
    if ! grep -a -q 'aresample' "$mpv_bin" || ! grep -a -q 'equalizer' "$mpv_bin"; then
      echo "fetch_full_libmpv: staged Mpv missing aresample/equalizer markers." >&2
      exit 1
    fi
    echo "Staged macOS full frameworks under $OUT"
    ;;
  Linux)
    OUT="$ROOT/third_party/libmpv/linux/x86_64"
    mkdir -p "$OUT"
    if compgen -G "$OUT/libmpv.so*" > /dev/null; then
      echo "Found bundled libs in $OUT"
    else
      echo "Linux: install a full distro libmpv (with lavfi filters), or place libmpv.so* in:"
      echo "  $OUT"
      if command -v pkg-config >/dev/null && pkg-config --exists mpv; then
        echo "System mpv: $(pkg-config --modversion mpv) at $(pkg-config --variable=libdir mpv)"
      fi
    fi
    ;;
  *)
    echo "Unsupported OS: $os (use tool/fetch_full_libmpv.ps1 on Windows)" >&2
    exit 1
    ;;
esac
