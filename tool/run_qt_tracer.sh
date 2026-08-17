#!/usr/bin/env bash
# Run the Qt Tramp host on the current session (Wayland if present).
# Do not source tramp-flutter-env.sh — that forces X11 for Flutter.
set -euo pipefail
root="$(git -C "$(dirname "$0")" rev-parse --show-toplevel)"
bin="$root/qt/build/tramp"
if [[ ! -x $bin ]]; then
  echo "Build first: ./qt/build.sh   (or cmake -S qt -B qt/build && cmake --build qt/build)" >&2
  exit 1
fi
exec "$bin" "$@"
