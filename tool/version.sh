#!/usr/bin/env bash
# Print VERSION fields (source-able).
#   version=1.0
#   msix=1.0.0.0
# Store MSIX Identity.Version revision (fourth number) must be 0.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
version="$(head -n 1 "$ROOT/VERSION" | tr -d '\r[:space:]')"
# Every packager labels its artifact from here. An unreadable VERSION used to
# pad to msix=0.0.0.0, which is a shape the Store accepts, so a blank file
# bought a legal identity for the wrong release instead of a failed build.
if [[ ! "$version" =~ ^[0-9]+(\.[0-9]+)*$ ]]; then
  echo "version.sh: VERSION is '$version', not a dotted release number" >&2
  exit 1
fi
IFS=. read -r a b c _ <<<"$version"
a="${a:-0}"; b="${b:-0}"; c="${c:-0}"
printf 'version=%s\n' "$version"
printf 'msix=%s.%s.%s.0\n' "$a" "$b" "$c"
