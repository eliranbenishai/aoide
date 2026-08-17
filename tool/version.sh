#!/usr/bin/env bash
# Print VERSION fields (source-able).
#   version=0.1.0
#   msix=0.1.0.0
# Store MSIX Identity.Version revision (fourth number) must be 0.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
version="$(head -n 1 "$ROOT/VERSION" | tr -d '\r[:space:]')"
IFS=. read -r a b c _ <<<"$version"
a="${a:-0}"; b="${b:-0}"; c="${c:-0}"
printf 'version=%s\n' "$version"
printf 'msix=%s.%s.%s.0\n' "$a" "$b" "$c"
