# Qt host (the Tramp app)

Five frameless windows in one process. QWidget + QPainter. This directory is
the **product binary** (`tramp`).

Live session: libmpv playback, playlist / collection, audible EQ (`af` lavfi),
real 20-bar spectrum (offline PCM + STFT), settings persistence, docking, file
open (picker, DnD, argv). Title-strip drag uses `startSystemMove()`. Extras skip
the taskbar. Closing **Tramp** (main) quits; extra close hides. `--dump-chrome DIR`
writes 1× logical PNGs from the golden fixture.

Windows: CI compiles this target. Drag feel on Windows comes later.

## Build (this host)

```sh
./qt/build.sh
./qt/build/tramp
```

`build.sh` is for machines without CMake on `PATH` (Homebrew Qt + clang). It
links the bundled full libmpv and SONAME stubs under `src/mpv_stubs/` when the
OS does not ship mujs/lua/uchardet/vapoursynth/Xpresent.

## Build (system Qt + CMake)

Fedora:

```sh
sudo dnf install qt6-qtbase-devel qt6-qtwayland libX11-devel cmake ninja-build gcc-c++ mpv-libs-devel
```

Arch:

```sh
sudo pacman -S qt6-base qt6-wayland cmake ninja gcc mpv
```

```sh
cmake -S qt -B qt/build -G Ninja
cmake --build qt/build
ctest --test-dir qt/build --output-on-failure
./qt/build/tramp
```

CMake prefers `pkg-config mpv`. If that is missing and
`third_party/libmpv/linux/x86_64/libmpv.so` exists, it links the bundle plus
stubs. Without either, the binary still runs with a silent `NullEngine`.

X11 (proven dock follow):

```sh
QT_QPA_PLATFORM=xcb ./qt/build/tramp
```

Dump logical chrome (no windows) for golden diffs:

```sh
QT_QPA_PLATFORM=offscreen ./qt/build/tramp --dump-chrome /tmp/tramp-chrome
```

This binary is Qt; let it see `WAYLAND_DISPLAY` unless you opt into xcb.

Drag the title strip (not the window buttons) to move a window. Closing **Tramp**
(main) quits the process.
