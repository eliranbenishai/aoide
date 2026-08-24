#!/usr/bin/env bash
# Emit GITHUB_ENV lines for the pin in QT_VERSION. Workflows source this
# instead of repeating the version string.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ver="$(tr -d '\r[:space:]' < "$ROOT/QT_VERSION")"
if [[ ! "$ver" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
  echo "QT_VERSION is not x.y.z: '$ver'" >&2
  exit 1
fi
echo "QT_VERSION=$ver"
echo "QT_RUNTIME=${ver%.*}"
