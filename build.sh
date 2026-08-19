#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
QT="${QT:-/home/linuxbrew/.linuxbrew/opt/qtbase}"
MOC="${MOC:-/home/linuxbrew/.linuxbrew/Cellar/qtbase/6.11.1/share/qt/libexec/moc}"
CXX="${CXX:-/home/linuxbrew/.linuxbrew/opt/llvm/bin/clang++}"
CC="${CC:-/home/linuxbrew/.linuxbrew/opt/llvm/bin/clang}"
BREW="${BREW:-/home/linuxbrew/.linuxbrew}"
MPV_INC="$ROOT/third_party/libmpv/include"
MPV_LIB="$ROOT/third_party/libmpv/linux/x86_64"
BUILD="$ROOT/build"
STUB="$BUILD/mpv-stubs"
STUB_SRC="$ROOT/src/mpv_stubs"
mkdir -p "$BUILD" "$STUB"
if [[ -d "$STUB_SRC" ]]; then
  for spec in libmujs.so.0.1 liblua-5.1.so libuchardet.so.0 libvapoursynth-script.so.0 libXpresent.so.1; do
    "$CC" -shared -fPIC -Wl,-soname,"$spec" -o "$STUB/$spec" "$STUB_SRC/$spec.c"
  done
fi

INC=(
  -I"$QT/include" -I"$QT/include/QtWidgets" -I"$QT/include/QtGui" -I"$QT/include/QtCore" \
  -I"$QT/include/QtDBus"
  -I"$MPV_INC"
  -I"$ROOT/src" -I"$BUILD"
)
VERSION="$(head -n 1 "$ROOT/VERSION" | tr -d '\r[:space:]')"
DEFS=(
  -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB -DQT_DBUS_LIB -DTRAMP_HAVE_MPV -DTRAMP_HAVE_DBUS
  -DTRAMP_VERSION="\"$VERSION\""
  -DTRAMP_ASSET_DIR="\"$ROOT/assets\""
  -DTRAMP_SKINS_DIR="\"$ROOT/skins\""
)
LIBS=(
  -L"$QT/lib" -L"$BREW/lib" -L"$MPV_LIB" -L"$STUB"
  -lQt6Widgets -lQt6Gui -lQt6Core -lQt6DBus -lmpv -lstdc++ -lm -lgcc_s -pthread
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

"$MOC" "$ROOT/src/host_window.h" -o "$BUILD/moc_host_window.cpp"
"$MOC" "$ROOT/src/host_shell_window.h" -o "$BUILD/moc_host_shell_window.cpp"
"$MOC" "$ROOT/src/session.h" -o "$BUILD/moc_session.cpp"
"$MOC" "$ROOT/src/mpv_engine.h" -o "$BUILD/moc_mpv_engine.cpp"
"$MOC" "$ROOT/src/native_file_dialog_p.h" -o "$BUILD/moc_native_file_dialog_p.cpp"

SRCS=(
  "$ROOT/src/window_spec.cpp"
  "$ROOT/src/title_chrome.cpp"
  "$ROOT/src/host_shell.cpp"
  "$ROOT/src/app_icon.cpp"
  "$ROOT/src/host_shell_window.cpp"
  "$ROOT/src/mockup_draw.cpp"
  "$ROOT/src/tramp_fonts.cpp"
  "$ROOT/src/chrome_paint.cpp"
  "$ROOT/src/chrome_bodies.cpp"
  "$ROOT/src/chrome_hits.cpp"
  "$ROOT/src/session_view.cpp"
  "$ROOT/src/m3u.cpp"
  "$ROOT/src/equalizer.cpp"
  "$ROOT/src/support_dir.cpp"
  "$ROOT/src/playlist.cpp"
  "$ROOT/src/transport.cpp"
  "$ROOT/src/settings.cpp"
  "$ROOT/src/persist.cpp"
  "$ROOT/src/collection.cpp"
  "$ROOT/src/files.cpp"
  "$ROOT/src/native_file_dialog.cpp"
  "$BUILD/moc_native_file_dialog_p.cpp"
  "$ROOT/src/playback.cpp"
  "$ROOT/src/mpv_engine.cpp"
  "$ROOT/src/pcm_decoder.cpp"
  "$ROOT/src/wav_reader.cpp"
  "$ROOT/src/stft.cpp"
  "$ROOT/src/spectrum.cpp"
  "$ROOT/src/docking.cpp"
  "$ROOT/src/look.cpp"
  "$ROOT/src/session.cpp"
  "$ROOT/src/host_window.cpp"
  "$ROOT/src/main.cpp"
  "$BUILD/moc_host_window.cpp"
  "$BUILD/moc_host_shell_window.cpp"
  "$BUILD/moc_session.cpp"
  "$BUILD/moc_mpv_engine.cpp"
)

"$CXX" "${CXXFLAGS[@]}" "${INC[@]}" "${DEFS[@]}" "${SRCS[@]}" "${LIBS[@]}" -o "$BUILD/tramp.next"
mv -f "$BUILD/tramp.next" "$BUILD/tramp"

# Domain tests (playlist / playback / docking / collection)
"$CXX" "${CXXFLAGS[@]}" "${INC[@]}" "${DEFS[@]}" \
  "$ROOT/src/m3u.cpp" "$ROOT/src/equalizer.cpp" "$ROOT/src/support_dir.cpp" \
  "$ROOT/src/playlist.cpp" "$ROOT/src/transport.cpp" \
  "$ROOT/src/wav_reader.cpp" "$ROOT/src/stft.cpp" "$ROOT/src/spectrum.cpp" \
  "$ROOT/src/playback.cpp" "$ROOT/src/docking.cpp" \
  "$ROOT/src/collection.cpp" "$ROOT/src/persist.cpp" "$ROOT/src/settings.cpp" \
  "$ROOT/src/window_spec.cpp" \
  "$ROOT/tests/domain_test.cpp" \
  -L"$QT/lib" -lQt6Widgets -lQt6Gui -lQt6Core -lstdc++ -lm -lgcc_s -pthread \
  -Wl,-rpath,"$QT/lib" \
  -o "$BUILD/domain_test"
"$BUILD/domain_test"

"$CXX" "${CXXFLAGS[@]}" "${INC[@]}" -DQT_GUI_LIB -DQT_CORE_LIB \
  -DTRAMP_SKINS_DIR="\"$ROOT/skins\"" \
  "$ROOT/src/look.cpp" "$ROOT/src/settings.cpp" "$ROOT/src/equalizer.cpp" \
  "$ROOT/src/tramp_fonts.cpp" \
  "$ROOT/tests/look_test.cpp" \
  -L"$QT/lib" -lQt6Gui -lQt6Core -lstdc++ -lm -lgcc_s -Wl,-rpath,"$QT/lib" \
  -o "$BUILD/look_test"
"$BUILD/look_test"

"$CXX" "${CXXFLAGS[@]}" "${INC[@]}" -DQT_GUI_LIB -DQT_CORE_LIB \
  -DTRAMP_ASSET_DIR="\"$ROOT/assets\"" -DTRAMP_SKINS_DIR="\"$ROOT/skins\"" \
  "$ROOT/src/tramp_fonts.cpp" \
  "$ROOT/tests/font_metrics_test.cpp" \
  -L"$QT/lib" -lQt6Gui -lQt6Core -lstdc++ -lm -lgcc_s -pthread -Wl,-rpath,"$QT/lib" \
  -o "$BUILD/font_metrics_test"
QT_QPA_PLATFORM=offscreen "$BUILD/font_metrics_test"

"$MOC" "$ROOT/tests/window_spec_test.cpp" -o "$BUILD/window_spec_test.moc"
"$CXX" "${CXXFLAGS[@]}" "${INC[@]}" -I"$QT/include/QtTest" -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB \
  "$ROOT/src/window_spec.cpp" \
  "$ROOT/tests/window_spec_test.cpp" \
  -L"$QT/lib" -lQt6Test -lQt6Widgets -lQt6Gui -lQt6Core -lstdc++ -lm -lgcc_s -pthread -Wl,-rpath,"$QT/lib" \
  -o "$BUILD/window_spec_test"
"$BUILD/window_spec_test"

"$MOC" "$ROOT/tests/host_shell_test.cpp" -o "$BUILD/host_shell_test.moc"
"$CXX" "${CXXFLAGS[@]}" "${INC[@]}" -I"$QT/include/QtTest" -DQT_GUI_LIB -DQT_CORE_LIB \
  "$ROOT/src/host_shell.cpp" \
  "$ROOT/tests/host_shell_test.cpp" \
  -L"$QT/lib" -lQt6Test -lQt6Gui -lQt6Core -lstdc++ -lm -lgcc_s -pthread -Wl,-rpath,"$QT/lib" \
  -o "$BUILD/host_shell_test"
"$BUILD/host_shell_test"

"$MOC" "$ROOT/src/host_shell_window.h" -o "$BUILD/moc_host_shell_window.cpp"
"$MOC" "$ROOT/tests/host_shell_window_test.cpp" -o "$BUILD/host_shell_window_test.moc"
"$CXX" "${CXXFLAGS[@]}" "${INC[@]}" -I"$QT/include/QtTest" -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB \
  -DTRAMP_ASSET_DIR="\"$ROOT/assets\"" \
  "$ROOT/src/window_spec.cpp" "$ROOT/src/host_shell.cpp" "$ROOT/src/host_shell_window.cpp" \
  "$ROOT/src/app_icon.cpp" "$ROOT/src/tramp_fonts.cpp" \
  "$BUILD/moc_host_shell_window.cpp" \
  "$ROOT/tests/host_shell_window_test.cpp" \
  -L"$QT/lib" -lQt6Test -lQt6Widgets -lQt6Gui -lQt6Core -lstdc++ -lm -lgcc_s -pthread -Wl,-rpath,"$QT/lib" \
  -o "$BUILD/host_shell_window_test"
QT_QPA_PLATFORM=offscreen "$BUILD/host_shell_window_test"

"$MOC" "$ROOT/src/host_window.h" -o "$BUILD/moc_host_window.cpp"
"$MOC" "$ROOT/tests/host_window_move_test.cpp" -o "$BUILD/host_window_move_test.moc"
"$CXX" "${CXXFLAGS[@]}" "${INC[@]}" -I"$QT/include/QtTest" \
  -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB \
  -DTRAMP_ASSET_DIR="\"$ROOT/assets\"" -DTRAMP_SKINS_DIR="\"$ROOT/skins\"" \
  "$ROOT/src/window_spec.cpp" "$ROOT/src/title_chrome.cpp" \
  "$ROOT/src/host_shell.cpp" "$ROOT/src/host_shell_window.cpp" \
  "$ROOT/src/app_icon.cpp" \
  "$ROOT/src/host_window.cpp" "$ROOT/src/chrome_paint.cpp" \
  "$ROOT/src/chrome_bodies.cpp" "$ROOT/src/chrome_hits.cpp" \
  "$ROOT/src/mockup_draw.cpp" "$ROOT/src/tramp_fonts.cpp" \
  "$ROOT/src/session_view.cpp" "$ROOT/src/look.cpp" \
  "$ROOT/src/settings.cpp" "$ROOT/src/equalizer.cpp" \
  "$BUILD/moc_host_shell_window.cpp" "$BUILD/moc_host_window.cpp" \
  "$ROOT/tests/host_window_move_test.cpp" \
  -L"$QT/lib" -lQt6Test -lQt6Widgets -lQt6Gui -lQt6Core -lstdc++ -lm -lgcc_s -pthread \
  -Wl,-rpath,"$QT/lib" \
  -o "$BUILD/host_window_move_test"
QT_QPA_PLATFORM=offscreen "$BUILD/host_window_move_test"

"$MOC" "$ROOT/tests/chrome_spec_test.cpp" -o "$BUILD/chrome_spec_test.moc"
"$CXX" "${CXXFLAGS[@]}" "${INC[@]}" -I"$QT/include/QtTest" -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB \
  "$ROOT/src/window_spec.cpp" "$ROOT/src/title_chrome.cpp" \
  "$ROOT/tests/chrome_spec_test.cpp" \
  -L"$QT/lib" -lQt6Test -lQt6Widgets -lQt6Gui -lQt6Core -lstdc++ -lm -lgcc_s -pthread -Wl,-rpath,"$QT/lib" \
  -o "$BUILD/chrome_spec_test"
"$BUILD/chrome_spec_test"

echo "built $BUILD/tramp"
