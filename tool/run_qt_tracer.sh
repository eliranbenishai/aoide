#!/usr/bin/env bash
# Run Tramp on the current session (Wayland if present).
set -euo pipefail
root="$(git -C "$(dirname "$0")" rev-parse --show-toplevel)"
bin="$root/build/tramp"
if [[ ! -x $bin ]]; then
  echo "Build first: ./build.sh   (or cmake -S . -B build && cmake --build build)" >&2
  exit 1
fi
# libmpv's nested DT_NEEDED (mujs/lua/…) do not inherit tramp's RUNPATH.
stub="$root/build/mpv-stubs"
if [[ -d $stub ]]; then
  export LD_LIBRARY_PATH="$stub${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
fi
exec "$bin" "$@"
