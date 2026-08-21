#!/usr/bin/env bash
# Quit under load, under AddressSanitizer.
#
# Tramp's background workers (spectrum decode, path verify, duration probe) used
# to be detached threads holding a QPointer to the session. Nothing waited for
# them, so a quit that landed while one was finishing marshalled a queued call
# into a QObject that was already gone. The window is a few instructions wide,
# which is why the audit could reason about it but not show it — so this runs the
# teardown thousands of times with the moment swept across the workers' lifetime.
#
#   tool/quit-under-load.sh            # both phases
#   tool/quit-under-load.sh loop       # in-process: session built and destroyed in a loop
#   tool/quit-under-load.sh app        # real launches with TRAMP_AUTO_QUIT=1
#
# Env: TRAMP_QUIT_ROUNDS, TRAMP_QUIT_LAUNCHES, TRAMP_QUIT_TRACKS, TRAMP_QUIT_SECONDS,
#      TRAMP_QUIT_SWEEP_MS, TRAMP_QUIT_LIVE_MS, TRAMP_QUIT_SKIP_BUILD, TRAMP_OPT.
#
# To re-prove the original crash on a tree without the fix, widen the gap the
# worker leaves between asking whether the session is alive and acting on the
# answer — that gap is the bug, a sleep only makes it visible:
#
#   in TrampSession::schedulePathVerify, between `if (!session) return;` and the
#   `QMetaObject::invokeMethod(...)` under it, add
#       std::this_thread::sleep_for(std::chrono::milliseconds(40));
#   then run this with TRAMP_QUIT_LIVE_MS set near the verify's own runtime.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
QT="${QT:-/home/linuxbrew/.linuxbrew/opt/qtbase}"
MOC="${MOC:-/home/linuxbrew/.linuxbrew/Cellar/qtbase/6.11.1/share/qt/libexec/moc}"
CXX="${CXX:-/home/linuxbrew/.linuxbrew/opt/llvm/bin/clang++}"
CC="${CC:-/home/linuxbrew/.linuxbrew/opt/llvm/bin/clang}"
BREW="${BREW:-/home/linuxbrew/.linuxbrew}"
MPV_INC="$ROOT/third_party/libmpv/include"
MPV_LIB="$ROOT/third_party/libmpv/linux/x86_64"
BUILD="$ROOT/build"
STUB="$BUILD/mpv-stubs"
WORK="$BUILD/quit-load"
PHASE="${1:-both}"

TRACKS="${TRAMP_QUIT_TRACKS:-60000}"
SECONDS_OF_AUDIO="${TRAMP_QUIT_SECONDS:-240}"
ROUNDS="${TRAMP_QUIT_ROUNDS:-24}"
LAUNCHES="${TRAMP_QUIT_LAUNCHES:-40}"

mkdir -p "$WORK" "$STUB"

# --- sanitized build -------------------------------------------------------
# Same flags as build.sh, plus the sanitizers. -O1 keeps the stacks readable;
# the paint budget does not apply to a teardown loop.
INC=(-I"$QT/include" -I"$QT/include/QtWidgets" -I"$QT/include/QtGui" -I"$QT/include/QtCore"
     -I"$QT/include/QtDBus" -I"$MPV_INC" -I"$ROOT/src" -I"$BUILD")
VERSION="$(head -n 1 "$ROOT/VERSION" | tr -d '\r[:space:]')"
DEFS=(-DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB -DQT_DBUS_LIB -DTRAMP_HAVE_MPV -DTRAMP_HAVE_DBUS
      -DTRAMP_VERSION="\"$VERSION\"" -DTRAMP_ASSET_DIR="\"$ROOT/assets\""
      -DTRAMP_SKINS_DIR="\"$ROOT/skins\"")
SAN=(-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=undefined -g)
CXXFLAGS=(-std=c++17 "${TRAMP_OPT:--O1}" -fPIC -Wall -Wextra -Wno-unused-parameter "${SAN[@]}")
LIBS=(-L"$QT/lib" -L"$BREW/lib" -L"$MPV_LIB" -L"$STUB"
      -lQt6Widgets -lQt6Gui -lQt6Core -lQt6DBus -lmpv -lstdc++ -lm -lgcc_s -pthread
      -Wl,--no-as-needed
      "$STUB/libmujs.so.0.1" "$STUB/liblua-5.1.so" "$STUB/libuchardet.so.0"
      "$STUB/libvapoursynth-script.so.0" "$STUB/libXpresent.so.1"
      -Wl,--as-needed -Wl,--allow-shlib-undefined
      -Wl,-rpath,"$QT/lib" -Wl,-rpath,"$BREW/lib" -Wl,-rpath,"$MPV_LIB" -Wl,-rpath,"$STUB")

APP_SRCS=(
  "$ROOT/src/window_spec.cpp" "$ROOT/src/title_chrome.cpp" "$ROOT/src/host_shell.cpp"
  "$ROOT/src/app_icon.cpp" "$ROOT/src/host_shell_window.cpp" "$ROOT/src/mockup_draw.cpp"
  "$ROOT/src/tramp_fonts.cpp" "$ROOT/src/chrome_anim.cpp" "$ROOT/src/chrome_paint.cpp"
  "$ROOT/src/chrome_bodies.cpp" "$ROOT/src/chrome_hits.cpp" "$ROOT/src/chrome_tooltip.cpp"
  "$ROOT/src/chrome_menu.cpp" "$ROOT/src/session_view.cpp" "$ROOT/src/m3u.cpp"
  "$ROOT/src/equalizer.cpp" "$ROOT/src/support_dir.cpp" "$ROOT/src/wait_cursor.cpp"
  "$ROOT/src/playlist.cpp" "$ROOT/src/transport.cpp" "$ROOT/src/settings.cpp"
  "$ROOT/src/persist.cpp" "$ROOT/src/collection.cpp" "$ROOT/src/duration_probe.cpp"
  "$ROOT/src/files.cpp" "$ROOT/src/native_file_dialog.cpp" "$BUILD/moc_native_file_dialog_p.cpp"
  "$ROOT/src/playback.cpp" "$ROOT/src/mpv_engine.cpp" "$ROOT/src/pcm_decoder.cpp"
  "$ROOT/src/wav_reader.cpp" "$ROOT/src/stft.cpp" "$ROOT/src/spectrum.cpp"
  "$ROOT/src/docking.cpp" "$ROOT/src/look.cpp" "$ROOT/src/session.cpp"
  "$ROOT/src/host_window.cpp"
  "$BUILD/moc_host_window.cpp" "$BUILD/moc_host_shell_window.cpp" "$BUILD/moc_session.cpp"
  "$BUILD/moc_mpv_engine.cpp"
)

if [[ "${TRAMP_QUIT_SKIP_BUILD:-0}" != "1" ]]; then
  if [[ -d "$ROOT/src/mpv_stubs" ]]; then
    for spec in libmujs.so.0.1 liblua-5.1.so libuchardet.so.0 libvapoursynth-script.so.0 libXpresent.so.1; do
      [[ -e "$STUB/$spec" ]] || "$CC" -shared -fPIC -Wl,-soname,"$spec" -o "$STUB/$spec" "$ROOT/src/mpv_stubs/$spec.c"
    done
  fi
  "$MOC" "$ROOT/src/host_window.h" -o "$BUILD/moc_host_window.cpp"
  "$MOC" "$ROOT/src/host_shell_window.h" -o "$BUILD/moc_host_shell_window.cpp"
  "$MOC" "$ROOT/src/session.h" -o "$BUILD/moc_session.cpp"
  "$MOC" "$ROOT/src/mpv_engine.h" -o "$BUILD/moc_mpv_engine.cpp"
  "$MOC" "$ROOT/src/native_file_dialog_p.h" -o "$BUILD/moc_native_file_dialog_p.cpp"

  echo "quit-load: building sanitized loop"
  "$CXX" "${CXXFLAGS[@]}" "${INC[@]}" "${DEFS[@]}" "${APP_SRCS[@]}" \
    "$ROOT/tool/quit_loop_main.cpp" "${LIBS[@]}" -o "$WORK/quit_loop"
  if [[ "$PHASE" != "loop" ]]; then
    echo "quit-load: building sanitized app"
    "$CXX" "${CXXFLAGS[@]}" "${INC[@]}" "${DEFS[@]}" "${APP_SRCS[@]}" \
      "$ROOT/src/main.cpp" "${LIBS[@]}" -o "$WORK/tramp-asan"
  fi
fi

# --- load ------------------------------------------------------------------
# One real track so playback opens something and the spectrum decode has a whole
# file to chew through; the rest are paths that do not exist, so the path verify
# has tens of thousands of stats to get through before it reports.
TONE="$WORK/tone-${SECONDS_OF_AUDIO}s.wav"
if [[ ! -f "$TONE" ]]; then
  echo "quit-load: rendering ${SECONDS_OF_AUDIO}s of noise"
  ffmpeg -hide_banner -loglevel error -y \
    -f lavfi -i "anoisesrc=d=${SECONDS_OF_AUDIO}:c=pink:r=44100" \
    -ac 1 -ar 44100 -c:a pcm_s16le "$TONE"
fi

DROPS="$WORK/drops"
if [[ ! -d "$DROPS" ]]; then
  mkdir -p "$DROPS"
  for i in $(seq 1 12); do cp "$ROOT/tests/fixtures/probe-tone.mp3" "$DROPS/drop-$i.mp3"; done
fi

SUPPORT="$WORK/home/com.proximamagnifica.tramp"
rm -rf "$WORK/home"
mkdir -p "$SUPPORT"
python3 - "$SUPPORT" "$TRACKS" "$TONE" <<'PY'
import json, sys
support, count, tone = sys.argv[1], int(sys.argv[2]), sys.argv[3]
# Durations and titles are filled in so nothing here asks for a duration probe;
# the probe is driven separately from the dropped files.
tracks = [{"path": tone, "title": "Load tone", "artist": "Tramp", "durationMs": 240000}]
tracks += [{"path": "/nonexistent/tramp-quit-load/%06d.mp3" % i, "title": "Ghost %d" % i,
            "durationMs": 1000} for i in range(count)]
json.dump({"sourcePath": None, "tracks": tracks}, open(support + "/altered_playlist.json", "w"))
json.dump({"resumeLastSession": True, "confirmBeforeQuit": False},
          open(support + "/settings.json", "w"))
json.dump({"playingIndex": 0, "positionMs": 0, "wasPlaying": True},
          open(support + "/session_resume.json", "w"))
PY

export XDG_DATA_HOME="$WORK/home"
export QT_QPA_PLATFORM=offscreen
# Leaks are not what this is hunting, and neither Qt nor libmpv is instrumented.
export ASAN_OPTIONS="detect_leaks=0:abort_on_error=0:halt_on_error=1:detect_stack_use_after_return=1"
export UBSAN_OPTIONS="print_stacktrace=1:halt_on_error=1"

# --- phase: in-process rounds ---------------------------------------------
if [[ "$PHASE" == "loop" || "$PHASE" == "both" ]]; then
  echo "quit-load: $ROUNDS rounds, $TRACKS tracks, ${SECONDS_OF_AUDIO}s track"
  TRAMP_QUIT_ROUNDS="$ROUNDS" TRAMP_QUIT_DROP_DIR="$DROPS" "$WORK/quit_loop"
fi

# --- phase: real launches --------------------------------------------------
# TRAMP_AUTO_QUIT starts a real session, then quits on the next turn of the event
# loop — every worker is still going when the process tears itself down.
if [[ "$PHASE" == "app" || "$PHASE" == "both" ]]; then
  echo "quit-load: $LAUNCHES launches with TRAMP_AUTO_QUIT=1"
  for i in $(seq 1 "$LAUNCHES"); do
    if ! TRAMP_AUTO_QUIT=1 "$WORK/tramp-asan" >"$WORK/launch.log" 2>&1; then
      echo "quit-load: launch $i failed" >&2
      cat "$WORK/launch.log" >&2
      exit 1
    fi
    if grep -qE 'ERROR: (Address|Undefined)|runtime error:' "$WORK/launch.log"; then
      echo "quit-load: launch $i reported a sanitizer error" >&2
      cat "$WORK/launch.log" >&2
      exit 1
    fi
  done
  echo "quit-load: $LAUNCHES launches clean"
fi
