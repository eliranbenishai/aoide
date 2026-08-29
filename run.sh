#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
# App only — ./build.sh stays the full test gate.
"$ROOT/tool/build-app.sh"
exec "$ROOT/build/aoide" "$@"
