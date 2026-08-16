# Qt window host

Five frameless windows in one process. QWidget + QPainter.

**Now:** mockup chassis plus static window bodies (main / EQ / playlist / settings / about) painted from the frozen Flutter layout numbers and golden fixtures. `--dump-chrome DIR` writes 1× logical PNGs for comparison. Title-strip drag uses `startSystemMove()`. Extras skip the taskbar. Closing **Tramp** (main) quits.

**Not yet:** live controls, docking, libmpv, zoom stepping.

Windows: CI compiles this target. Drag feel on Windows comes later.

## Build (system Qt)

Fedora:

```sh
sudo dnf install qt6-qtbase-devel qt6-qtwayland libX11-devel cmake ninja-build gcc-c++
```

Arch:

```sh
sudo pacman -S qt6-base qt6-wayland cmake ninja gcc
```

```sh
cmake -S qt -B qt/build -G Ninja
cmake --build qt/build
ctest --test-dir qt/build --output-on-failure
```

Run (Wayland — default on Fedora):

```sh
./qt/build/tramp-qt-tracer
```

X11:

```sh
QT_QPA_PLATFORM=xcb ./qt/build/tramp-qt-tracer
```

Dump logical chrome (no windows) for golden diffs:

```sh
QT_QPA_PLATFORM=offscreen ./qt/build/tramp-qt-tracer --dump-chrome /tmp/tramp-chrome
```

Do not use `~/.config/tramp-flutter-env.sh` (it forces X11 and Mesa for Flutter). This binary is Qt; let it see `WAYLAND_DISPLAY`.

Drag the title strip (not the window buttons) to move a window. Closing **Tramp** (main) quits the process.
