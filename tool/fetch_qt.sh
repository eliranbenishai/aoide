#!/usr/bin/env bash
# Install the official desktop Qt named in QT_VERSION — same kit CI uses.
# Linux only; output is .local/qt/<version>/gcc_64 (gitignored).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
# shellcheck source=qt-env.sh
source "$ROOT/tool/qt-env.sh"
tramp_read_qt_pin
PREFIX="$(tramp_qt_prefix)"

if [[ "$(uname -s)" != Linux ]]; then
  echo "fetch_qt.sh installs the Linux desktop kit. On Windows, CI uses install-qt-action." >&2
  exit 1
fi

if [[ -e "$PREFIX/lib/libQt6Core.so" || -e "$PREFIX/lib/libQt6Core.so.6" ]]; then
  echo "Qt $TRAMP_QT_VERSION already at $PREFIX"
  exit 0
fi

VENV="$ROOT/.local/aqt-venv"
mkdir -p "$ROOT/.local"

pick_python() {
  local c
  for c in /usr/bin/python3 /usr/bin/python3.14 /usr/bin/python; do
    if [[ -x "$c" ]]; then
      echo "$c"
      return
    fi
  done
  if command -v python3 >/dev/null; then
    command -v python3
    return
  fi
  echo "python3 is required to run aqtinstall" >&2
  exit 1
}

if [[ ! -x "$VENV/bin/python" ]]; then
  "$(pick_python)" -m venv "$VENV"
fi
"$VENV/bin/python" -m pip install -q -U pip
"$VENV/bin/python" -m pip install -q 'aqtinstall==3.3.0'

echo "Installing Qt $TRAMP_QT_VERSION (qtbase qtwayland icu) into $ROOT/.local/qt"
# linux_gcc_64 is the 6.7+ arch name. qtwayland is a base archive, not a module.
"$VENV/bin/python" -m aqt install-qt linux desktop "$TRAMP_QT_VERSION" linux_gcc_64 \
  --outputdir "$ROOT/.local/qt" \
  --archives qtbase qtwayland icu

if [[ ! -e "$PREFIX/lib/libQt6Core.so" && ! -e "$PREFIX/lib/libQt6Core.so.6" ]]; then
  echo "aqt finished but $PREFIX has no libQt6Core" >&2
  exit 1
fi
echo "Qt $TRAMP_QT_VERSION ready at $PREFIX"
