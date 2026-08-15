#!/usr/bin/env bash
# Print pubspec.yaml version fields (source-able).
#   version=0.1.0
#   build=1
#   msix=0.1.0.0
# Store MSIX Identity.Version revision (fourth number) must be 0.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
line="$(sed -n 's/^version:[[:space:]]*//p' "$ROOT/pubspec.yaml" | head -n 1 | tr -d '\r')"
version="${line%%+*}"
build="${line#*+}"
if [[ "$build" == "$line" ]]; then
  build=0
fi
IFS=. read -r a b c _ <<<"$version"
a="${a:-0}"; b="${b:-0}"; c="${c:-0}"
printf 'version=%s\n' "$version"
printf 'build=%s\n' "$build"
printf 'msix=%s.%s.%s.0\n' "$a" "$b" "$c"
