#!/usr/bin/env bash
# Copy the system full libmpv (and its non-libc NEEDED libs) into
# third_party/libmpv/linux/x86_64 so CMake installs them into the bundle.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/third_party/libmpv/linux/x86_64"
mkdir -p "$OUT"

if ! command -v pkg-config >/dev/null || ! pkg-config --exists mpv; then
  echo "stage_linux_libmpv: libmpv not found (install libmpv-dev)" >&2
  exit 1
fi

libdir="$(pkg-config --variable=libdir mpv)"
shopt -s nullglob
sos=("$libdir"/libmpv.so*)
if [[ ${#sos[@]} -eq 0 ]]; then
  echo "stage_linux_libmpv: no libmpv.so* in $libdir" >&2
  exit 1
fi
cp -a "${sos[@]}" "$OUT/"

# Walk DT_NEEDED of libmpv so AppImage/bundle is not one .so short of FFmpeg.
primary="$(readlink -f "${sos[0]}")"
while read -r dep; do
  base="$(basename "$dep")"
  case "$base" in
    libc.so*|libm.so*|libdl.so*|libpthread.so*|libgcc_s.so*|ld-linux*.so*|libstdc++.so*)
      continue
      ;;
  esac
  if [[ -f "$dep" ]]; then
    cp -a "$dep" "$OUT/" || true
  fi
done < <(ldd "$primary" | awk '/=>/ { print $3 }' | grep -v '^$')

echo "stage_linux_libmpv: staged into $OUT"
ls -l "$OUT"
