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
    # The Windows fetcher greps one monolithic DLL, so it can ask libmpv-2.dll
    # about FFmpeg. Here FFmpeg is separate frameworks and Mpv answers neither
    # question: 'aresample' is a libavfilter filter and lives only in Avfilter,
    # and the FFmpeg configure line lives in Avutil/Avcodec/Avfilter. Grepping
    # Mpv alone failed every fetch on the resample marker while the slim check
    # silently passed whatever it was given. Search the staged set instead.
    if [[ ! -d "$OUT/Avfilter.xcframework" ]]; then
      echo "fetch_full_libmpv: no Avfilter.xcframework — no filter graph, so no EQ." >&2
      exit 1
    fi
    staged_bins=()
    shopt -s nullglob
    for fwdir in "$OUT"/*.xcframework/macos-*/*.framework; do
      fw_name="$(basename "$fwdir")"
      fw_name="${fw_name%.framework}"
      [[ -f "$fwdir/Versions/A/$fw_name" ]] && staged_bins+=("$fwdir/Versions/A/$fw_name")
    done
    shopt -u nullglob
    if ((${#staged_bins[@]} == 0)); then
      echo "fetch_full_libmpv: staged xcframeworks carry no framework binaries" >&2
      exit 1
    fi
    staged_has() {
      local needle="$1" b
      for b in "${staged_bins[@]}"; do
        grep -a -q -- "$needle" "$b" && return 0
      done
      return 1
    }
    if staged_has '--disable-filters'; then
      echo "fetch_full_libmpv: staged FFmpeg is slim (--disable-filters). Refusing." >&2
      exit 1
    fi
    for marker in aresample equalizer; do
      if ! staged_has "$marker"; then
        echo "fetch_full_libmpv: no '$marker' in any staged framework — slim build. Refusing." >&2
        exit 1
      fi
    done
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
