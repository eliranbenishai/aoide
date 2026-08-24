#!/usr/bin/env bash
# Build and run one test target with the same compiler and flags as ./build.sh,
# for a tight red-green loop. ./build.sh remains the gate.
#
#   tool/build-test.sh domain
#   tool/build-test.sh look
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
# shellcheck source=qt-env.sh
source "$ROOT/tool/qt-env.sh"
tramp_resolve_qt
CXX="${CXX:-/home/linuxbrew/.linuxbrew/opt/llvm/bin/clang++}"
BREW="${BREW:-/home/linuxbrew/.linuxbrew}"
MPV_INC="$ROOT/third_party/libmpv/include"
MPV_LIB="$ROOT/third_party/libmpv/linux/x86_64"
BUILD="$ROOT/build"
STUB="$BUILD/mpv-stubs"
NAME="${1:-domain}"

INC=(-I"$QT/include" -I"$QT/include/QtWidgets" -I"$QT/include/QtGui" -I"$QT/include/QtCore"
     -I"$MPV_INC" -I"$ROOT/src" -I"$BUILD")
CXXFLAGS=(-std=c++17 "${TRAMP_OPT:--O2}" -fPIC -Wall -Wextra -Wno-unused-parameter)

case "$NAME" in
  domain)
    "$CXX" "${CXXFLAGS[@]}" "${INC[@]}" -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB \
      -DTRAMP_HAVE_MPV \
      "$ROOT/src/m3u.cpp" "$ROOT/src/equalizer.cpp" "$ROOT/src/support_dir.cpp" \
      "$ROOT/src/wait_cursor.cpp" "$ROOT/src/playlist.cpp" "$ROOT/src/transport.cpp" \
      "$ROOT/src/wav_reader.cpp" "$ROOT/src/stft.cpp" "$ROOT/src/spectrum.cpp" \
      "$ROOT/src/playback.cpp" "$ROOT/src/docking.cpp" \
      "$ROOT/src/collection.cpp" "$ROOT/src/duration_probe.cpp" "$ROOT/src/persist.cpp" \
      "$ROOT/src/settings.cpp" "$ROOT/src/window_spec.cpp" \
      "$ROOT/tests/domain_test.cpp" \
      -L"$QT/lib" -L"$BREW/lib" -L"$MPV_LIB" -L"$STUB" \
      -lQt6Widgets -lQt6Gui -lQt6Core -lmpv -lstdc++ -lm -lgcc_s -pthread \
      -Wl,--no-as-needed \
      "$STUB/libmujs.so.0.1" "$STUB/liblua-5.1.so" "$STUB/libuchardet.so.0" \
      "$STUB/libvapoursynth-script.so.0" "$STUB/libXpresent.so.1" \
      -Wl,--as-needed -Wl,--allow-shlib-undefined \
      -Wl,-rpath,"$QT/lib" -Wl,-rpath,"$BREW/lib" -Wl,-rpath,"$MPV_LIB" -Wl,-rpath,"$STUB" \
      -o "$BUILD/domain_test"
    "$BUILD/domain_test"
    ;;
  look)
    "$CXX" "${CXXFLAGS[@]}" "${INC[@]}" -DQT_GUI_LIB -DQT_CORE_LIB \
      -DTRAMP_SKINS_DIR="\"$ROOT/skins\"" \
      "$ROOT/src/look.cpp" "$ROOT/src/settings.cpp" "$ROOT/src/equalizer.cpp" \
      "$ROOT/src/tramp_fonts.cpp" "$ROOT/tests/look_test.cpp" \
      -L"$QT/lib" -lQt6Gui -lQt6Core -lstdc++ -lm -lgcc_s -Wl,-rpath,"$QT/lib" \
      -o "$BUILD/look_test"
    "$BUILD/look_test"
    ;;
  *)
    echo "unknown test target: $NAME (try: domain, look)" >&2
    exit 2
    ;;
esac
