#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
QT="${QT:-/home/linuxbrew/.linuxbrew/opt/qtbase}"
MOC="${MOC:-/home/linuxbrew/.linuxbrew/Cellar/qtbase/6.11.1/share/qt/libexec/moc}"
CXX="${CXX:-/home/linuxbrew/.linuxbrew/opt/llvm/bin/clang++}"
CC="${CC:-/home/linuxbrew/.linuxbrew/opt/llvm/bin/clang}"
BREW="${BREW:-/home/linuxbrew/.linuxbrew}"
MPV_INC="$ROOT/third_party/libmpv/include"
MPV_LIB="$ROOT/third_party/libmpv/linux/x86_64"
BUILD="$ROOT/qt/build"
STUB="$BUILD/mpv-stubs"
STUB_SRC="$ROOT/qt/src/mpv_stubs"
mkdir -p "$BUILD" "$STUB"
if [[ -d "$STUB_SRC" ]]; then
  for spec in libmujs.so.0.1 liblua-5.1.so libuchardet.so.0 libvapoursynth-script.so.0 libXpresent.so.1; do
    "$CC" -shared -fPIC -Wl,-soname,"$spec" -o "$STUB/$spec" "$STUB_SRC/$spec.c"
  done
fi

INC=(
  -I"$QT/include" -I"$QT/include/QtWidgets" -I"$QT/include/QtGui" -I"$QT/include/QtCore"
  -I"$BREW/opt/libx11/include" -I"$BREW/opt/xorgproto/include"
  -I"$MPV_INC"
  -I"$ROOT/qt/src" -I"$BUILD"
)
VERSION="$(head -n 1 "$ROOT/VERSION" | tr -d '\r[:space:]')"
DEFS=(
  -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB -DTRAMP_HAVE_X11 -DTRAMP_HAVE_MPV
  -DTRAMP_VERSION="\"$VERSION\""
  -DTRAMP_ASSET_DIR="\"$ROOT/assets\""
  -DTRAMP_SKINS_DIR="\"$ROOT/skins\""
)
LIBS=(
  -L"$QT/lib" -L"$BREW/lib" -L"$MPV_LIB" -L"$STUB"
  -lQt6Widgets -lQt6Gui -lQt6Core -lmpv -lX11 -lstdc++ -lm -lgcc_s -pthread
  -Wl,--no-as-needed
  "$STUB/libmujs.so.0.1"
  "$STUB/liblua-5.1.so"
  "$STUB/libuchardet.so.0"
  "$STUB/libvapoursynth-script.so.0"
  "$STUB/libXpresent.so.1"
  -Wl,--as-needed
  -Wl,--allow-shlib-undefined
  -Wl,-rpath,"$QT/lib" -Wl,-rpath,"$BREW/lib" -Wl,-rpath,"$MPV_LIB" -Wl,-rpath,"$STUB"
)
CXXFLAGS=(-std=c++17 -fPIC -Wall -Wextra -Wno-unused-parameter)

"$MOC" "$ROOT/qt/src/host_window.h" -o "$BUILD/moc_host_window.cpp"
"$MOC" "$ROOT/qt/src/session.h" -o "$BUILD/moc_session.cpp"
"$MOC" "$ROOT/qt/src/mpv_engine.h" -o "$BUILD/moc_mpv_engine.cpp"

SRCS=(
  "$ROOT/qt/src/window_spec.cpp"
  "$ROOT/qt/src/title_chrome.cpp"
  "$ROOT/qt/src/skip_taskbar.cpp"
  "$ROOT/qt/src/mockup_draw.cpp"
  "$ROOT/qt/src/tramp_fonts.cpp"
  "$ROOT/qt/src/chrome_paint.cpp"
  "$ROOT/qt/src/chrome_bodies.cpp"
  "$ROOT/qt/src/chrome_hits.cpp"
  "$ROOT/qt/src/session_view.cpp"
  "$ROOT/qt/src/m3u.cpp"
  "$ROOT/qt/src/equalizer.cpp"
  "$ROOT/qt/src/support_dir.cpp"
  "$ROOT/qt/src/playlist.cpp"
  "$ROOT/qt/src/transport.cpp"
  "$ROOT/qt/src/settings.cpp"
  "$ROOT/qt/src/persist.cpp"
  "$ROOT/qt/src/collection.cpp"
  "$ROOT/qt/src/files.cpp"
  "$ROOT/qt/src/playback.cpp"
  "$ROOT/qt/src/mpv_engine.cpp"
  "$ROOT/qt/src/pcm_decoder.cpp"
  "$ROOT/qt/src/wav_reader.cpp"
  "$ROOT/qt/src/stft.cpp"
  "$ROOT/qt/src/spectrum.cpp"
  "$ROOT/qt/src/docking.cpp"
  "$ROOT/qt/src/look.cpp"
  "$ROOT/qt/src/session.cpp"
  "$ROOT/qt/src/host_window.cpp"
  "$ROOT/qt/src/main.cpp"
  "$BUILD/moc_host_window.cpp"
  "$BUILD/moc_session.cpp"
  "$BUILD/moc_mpv_engine.cpp"
)

"$CXX" "${CXXFLAGS[@]}" "${INC[@]}" "${DEFS[@]}" "${SRCS[@]}" "${LIBS[@]}" -o "$BUILD/tramp"

# Domain tests (no widgets / mpv)
"$CXX" "${CXXFLAGS[@]}" -I"$QT/include" -I"$QT/include/QtCore" -I"$ROOT/qt/src" -DQT_CORE_LIB \
  "$ROOT/qt/src/m3u.cpp" "$ROOT/qt/src/equalizer.cpp" "$ROOT/qt/src/support_dir.cpp" \
  "$ROOT/qt/src/playlist.cpp" "$ROOT/qt/src/transport.cpp" \
  "$ROOT/qt/src/wav_reader.cpp" "$ROOT/qt/src/stft.cpp" "$ROOT/qt/src/spectrum.cpp" \
  "$ROOT/qt/src/playback.cpp" \
  "$ROOT/qt/tests/domain_test.cpp" \
  -L"$QT/lib" -lQt6Core -lstdc++ -lm -lgcc_s -Wl,-rpath,"$QT/lib" \
  -o "$BUILD/domain_test"
"$BUILD/domain_test"

"$CXX" "${CXXFLAGS[@]}" "${INC[@]}" -DQT_GUI_LIB -DQT_CORE_LIB \
  "$ROOT/qt/src/look.cpp" "$ROOT/qt/src/settings.cpp" "$ROOT/qt/src/equalizer.cpp" \
  "$ROOT/qt/src/tramp_fonts.cpp" \
  "$ROOT/qt/tests/look_test.cpp" \
  -L"$QT/lib" -lQt6Gui -lQt6Core -lstdc++ -lm -lgcc_s -Wl,-rpath,"$QT/lib" \
  -o "$BUILD/look_test"
"$BUILD/look_test"

"$MOC" "$ROOT/qt/tests/chrome_spec_test.cpp" -o "$BUILD/chrome_spec_test.moc"
"$CXX" "${CXXFLAGS[@]}" "${INC[@]}" -I"$QT/include/QtTest" -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB \
  "$ROOT/qt/src/window_spec.cpp" "$ROOT/qt/src/title_chrome.cpp" \
  "$ROOT/qt/tests/chrome_spec_test.cpp" \
  -L"$QT/lib" -lQt6Test -lQt6Widgets -lQt6Gui -lQt6Core -lstdc++ -lm -lgcc_s -pthread -Wl,-rpath,"$QT/lib" \
  -o "$BUILD/chrome_spec_test"
"$BUILD/chrome_spec_test"

echo "built $BUILD/tramp"
